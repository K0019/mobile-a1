// resource/asset_loader.cpp
#include "asset_loader.h"
#include "processed_assets.h"
#include "asset_formats/mesh_format.h"
#include "asset_formats/anim_format.h"
#include "VFS/VFS.h"
#include "logging/log.h"
#include "renderer/gfx_interface.h"
#include "core_utils/util.h"

#include "ktx.h"
#include <cstring>
#include <cassert>
#include <map>

#undef RAPIDJSON_HAS_STDSTRING
#define RAPIDJSON_HAS_STDSTRING 1
#include "rapidjson/document.h"

namespace Resource {

// ============================================================================
// TEXTURE LOADING (.ktx2)
// ============================================================================

ProcessedTexture AssetLoader::LoadTexture(const std::string& vfsPath)
{
    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Texture file not found: {}", vfsPath);
        return ProcessedTexture{};
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read texture file failed: {}", vfsPath);
        return ProcessedTexture{};
    }

    std::string name = VFS::GetFilename(vfsPath);
    return ParseKTX2(fileData, name);
}

ProcessedTexture AssetLoader::ParseKTX2(const std::vector<uint8_t>& fileData, const std::string& name)
{
    ProcessedTexture texture{};
    texture.name = name;

    ktxTexture2* ktxTex = nullptr;

    KTX_error_code result = ktxTexture2_CreateFromMemory(
        fileData.data(),
        fileData.size(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &ktxTex
    );

    if (result != KTX_SUCCESS || !ktxTex)
    {
        LOG_ERROR("Failed to load KTX2 file {}. KTX Error: {}", name, ktxErrorString(result));
        if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
        return texture;
    }

    if (ktxTex->classId != class_id::ktxTexture2_c)
    {
        LOG_ERROR("File {} is not a KTX2 file. Found KTX1 instead.", name);
        ktxTexture_Destroy(ktxTexture(ktxTex));
        return texture;
    }

    // Auto-cleanup on scope exit
    SCOPE_EXIT { ktxTexture_Destroy(ktxTexture(ktxTex)); };

    // Extract texture properties
    texture.originalFileSize = fileData.size();
    texture.width = ktxTex->baseWidth;
    texture.height = ktxTex->baseHeight;
    texture.channels = 4; // Most KTX textures are 4-channel

    // Copy texture data
    const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
    texture.data.resize(dataSize);
    std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

    // Create descriptor
    texture.textureDesc = gfx::TextureMetadata{
        .type = gfx::TextureType::Tex2D,
        .format = gfx::vkFormatToFormat(static_cast<int>(ktxTex->vkFormat)),
        .dimensions = {texture.width, texture.height, 1},
        .numLayers = 1,
        .numMipLevels = ktxTex->numLevels,
        .usage = gfx::TextureUsage::Sampled,
        .debugName = name.c_str()
    };

    LOG_DEBUG("Loaded KTX2 texture {} - {}x{}, {} mips",
              name, texture.width, texture.height, ktxTex->numLevels);

    return texture;
}

// ============================================================================
// MESH LOADING (.mesh)
// ============================================================================

ProcessedMesh AssetLoader::LoadMesh(const std::string& vfsPath)
{
    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Mesh file not found: {}", vfsPath);
        return ProcessedMesh{};
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read mesh file failed: {}", vfsPath);
        return ProcessedMesh{};
    }

    return ParseMeshFile(fileData);
}

ProcessedMesh AssetLoader::ParseMeshFile(const std::vector<uint8_t>& fileData)
{
    if (fileData.size() < sizeof(MeshFileHeader))
    {
        LOG_ERROR("Mesh file too small to contain header");
        return ProcessedMesh{};
    }

    // Read header
    const MeshFileHeader* header = reinterpret_cast<const MeshFileHeader*>(fileData.data());

    if (header->magic != MESH_FILE_MAGIC)
    {
        LOG_ERROR("Invalid mesh file format (bad magic number)");
        return ProcessedMesh{};
    }

    // Read all data blocks
    const MeshNode* nodes = reinterpret_cast<const MeshNode*>(fileData.data() + header->nodeDataOffset);
    const MeshInfo* meshInfos = reinterpret_cast<const MeshInfo*>(fileData.data() + header->meshInfoDataOffset);
    const char* meshNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->meshNamesOffset);
    const char* materialNamesBuffer = reinterpret_cast<const char*>(fileData.data() + header->materialNamesOffset);
    const uint32_t* allIndices = reinterpret_cast<const uint32_t*>(fileData.data() + header->indexDataOffset);
    const MeshFile_Vertex* allVertices = reinterpret_cast<const MeshFile_Vertex*>(fileData.data() + header->vertexDataOffset);

    // Skeleton data (if present)
    const SkinningData* allSkinningData = nullptr;
    const MeshFile_Bone* allBones = nullptr;
    const char* boneNameBuffer = nullptr;
    if (header->hasSkeleton)
    {
        allSkinningData = reinterpret_cast<const SkinningData*>(fileData.data() + header->skinningDataOffset);
        allBones = reinterpret_cast<const MeshFile_Bone*>(fileData.data() + header->boneDataOffset);
        boneNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->boneNameOffset);
    }

    // Morph data (if present)
    const MeshFile_MorphTarget* allMorphTargets = nullptr;
    const MeshFile_MorphDelta* allMorphDeltas = nullptr;
    const char* morphTargetNameBuffer = nullptr;
    if (header->hasMorphs)
    {
        allMorphTargets = reinterpret_cast<const MeshFile_MorphTarget*>(fileData.data() + header->morphTargetDataOffset);
        allMorphDeltas = reinterpret_cast<const MeshFile_MorphDelta*>(fileData.data() + header->morphDeltaDataOffset);
        morphTargetNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->morphTargetNameOffset);
    }

    // Build skeleton (if present)
    ProcessedSkeleton skeleton;
    if (header->hasSkeleton)
    {
        skeleton.parentIndices.resize(header->numBones);
        skeleton.inverseBindMatrices.resize(header->numBones);
        skeleton.bindPoseMatrices.resize(header->numBones);
        skeleton.jointNames.resize(header->numBones);

        for (uint32_t i = 0; i < header->numBones; ++i)
        {
            const auto& fileBone = allBones[i];
            skeleton.parentIndices[i] = fileBone.parentIndex;
            skeleton.inverseBindMatrices[i] = fileBone.inverseBindPose;
            skeleton.bindPoseMatrices[i] = fileBone.bindPose;
            skeleton.jointNames[i] = &boneNameBuffer[fileBone.nameOffset];
        }
    }

    // Build morph targets (if present)
    std::vector<MorphTargetData> allMorphTargetsData;
    if (header->hasMorphs)
    {
        allMorphTargetsData.resize(header->numMorphTargets);
        for (uint32_t i = 0; i < header->numMorphTargets; ++i)
        {
            const auto& fileTarget = allMorphTargets[i];
            auto& target = allMorphTargetsData[i];

            target.name = &morphTargetNameBuffer[fileTarget.nameOffset];
            target.deltas.resize(fileTarget.deltaCount);

            for (uint32_t j = 0; j < fileTarget.deltaCount; ++j)
            {
                const auto& fileDelta = allMorphDeltas[fileTarget.firstDelta + j];
                auto& delta = target.deltas[j];

                delta.vertexIndex = fileDelta.vertexIndex;
                delta.deltaPosition = fileDelta.deltaPosition;
                delta.deltaNormal = fileDelta.deltaNormal;
                delta.deltaTangent = fileDelta.deltaTangent;
            }
        }
    }

    // Extract FIRST mesh only (TODO: support multi-mesh files)
    if (header->numMeshes == 0)
    {
        LOG_WARNING("Mesh file contains no meshes");
        return ProcessedMesh{};
    }

    const MeshInfo& meshInfo = meshInfos[0];
    ProcessedMesh mesh;
    mesh.name = &meshNameBuffer[meshInfo.nameOffset];
    mesh.materialIndex = 0;  // Will be resolved by material loading

    // Calculate vertex count for this mesh
    uint32_t vertexCount = (header->numMeshes > 1)
        ? (meshInfos[1].firstVertex - meshInfo.firstVertex)
        : (header->totalVertices - meshInfo.firstVertex);

    // Extract vertices - MeshFile_Vertex and Vertex have identical layouts
    static_assert(sizeof(MeshFile_Vertex) == sizeof(Vertex), "Vertex format mismatch");
    const Vertex* srcVertices = reinterpret_cast<const Vertex*>(allVertices + meshInfo.firstVertex);
    mesh.vertices.assign(srcVertices, srcVertices + vertexCount);

    // Extract indices (convert from global to local)
    mesh.indices.reserve(meshInfo.indexCount);
    for (uint32_t i = 0; i < meshInfo.indexCount; ++i)
    {
        uint32_t globalIndex = allIndices[meshInfo.firstIndex + i];
        uint32_t localIndex = globalIndex - meshInfo.firstVertex;
        mesh.indices.push_back(localIndex);
    }

    mesh.bounds = meshInfo.meshBounds;

    // Extract skinning data
    if (header->hasSkeleton)
    {
        mesh.skinning.assign(allSkinningData + meshInfo.firstVertex,
                            allSkinningData + meshInfo.firstVertex + vertexCount);
        mesh.skeleton = skeleton;
    }

    // Extract morph targets and remap to local indices
    if (header->hasMorphs && meshInfo.morphTargetCount > 0)
    {
        mesh.morphTargets.assign(allMorphTargetsData.begin() + meshInfo.firstMorphTarget,
                                allMorphTargetsData.begin() + meshInfo.firstMorphTarget + meshInfo.morphTargetCount);

        // Remap vertex indices from global to local
        for (auto& target : mesh.morphTargets)
        {
            std::vector<MorphTargetVertexDelta> localDeltas;
            for (const auto& delta : target.deltas)
            {
                if (delta.vertexIndex >= meshInfo.firstVertex &&
                    delta.vertexIndex < (meshInfo.firstVertex + vertexCount))
                {
                    MorphTargetVertexDelta localDelta = delta;
                    localDelta.vertexIndex = delta.vertexIndex - meshInfo.firstVertex;
                    localDeltas.push_back(localDelta);
                }
            }
            target.deltas = std::move(localDeltas);
        }
    }

    LOG_DEBUG("Loaded mesh '{}' - {} vertices, {} indices", mesh.name, mesh.vertices.size(), mesh.indices.size());
    return mesh;
}

// ============================================================================
// MODEL LOADING (multi-submesh .mesh files)
// ============================================================================

ProcessedModel AssetLoader::LoadModel(const std::string& vfsPath, const std::string& materialsBasePath)
{
    ProcessedModel model;
    model.sourcePath = vfsPath;

    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Mesh file not found: {}", vfsPath);
        return model;
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read mesh file failed: {}", vfsPath);
        return model;
    }

    if (fileData.size() < sizeof(MeshFileHeader))
    {
        LOG_ERROR("Mesh file too small to contain header");
        return model;
    }

    const MeshFileHeader* header = reinterpret_cast<const MeshFileHeader*>(fileData.data());
    if (header->magic != MESH_FILE_MAGIC)
    {
        LOG_ERROR("Invalid mesh file format (bad magic number)");
        return model;
    }

    model.hasSkeleton = header->hasSkeleton;
    model.hasMorphs = header->hasMorphs;

    // Read data pointers
    const MeshNode* nodes = reinterpret_cast<const MeshNode*>(fileData.data() + header->nodeDataOffset);
    const MeshInfo* meshInfos = reinterpret_cast<const MeshInfo*>(fileData.data() + header->meshInfoDataOffset);
    const char* meshNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->meshNamesOffset);
    const char* materialNamesBuffer = reinterpret_cast<const char*>(fileData.data() + header->materialNamesOffset);
    const uint32_t* allIndices = reinterpret_cast<const uint32_t*>(fileData.data() + header->indexDataOffset);
    const MeshFile_Vertex* allVertices = reinterpret_cast<const MeshFile_Vertex*>(fileData.data() + header->vertexDataOffset);

    // Skeleton data
    const SkinningData* allSkinningData = nullptr;
    const MeshFile_Bone* allBones = nullptr;
    const char* boneNameBuffer = nullptr;
    if (header->hasSkeleton)
    {
        allSkinningData = reinterpret_cast<const SkinningData*>(fileData.data() + header->skinningDataOffset);
        allBones = reinterpret_cast<const MeshFile_Bone*>(fileData.data() + header->boneDataOffset);
        boneNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->boneNameOffset);

        // Build shared skeleton
        model.skeleton.parentIndices.resize(header->numBones);
        model.skeleton.inverseBindMatrices.resize(header->numBones);
        model.skeleton.bindPoseMatrices.resize(header->numBones);
        model.skeleton.jointNames.resize(header->numBones);

        for (uint32_t i = 0; i < header->numBones; ++i)
        {
            const auto& fileBone = allBones[i];
            model.skeleton.parentIndices[i] = fileBone.parentIndex;
            model.skeleton.inverseBindMatrices[i] = fileBone.inverseBindPose;
            model.skeleton.bindPoseMatrices[i] = fileBone.bindPose;
            model.skeleton.jointNames[i] = &boneNameBuffer[fileBone.nameOffset];
        }
    }

    // Morph data
    const MeshFile_MorphTarget* allMorphTargets = nullptr;
    const MeshFile_MorphDelta* allMorphDeltas = nullptr;
    const char* morphTargetNameBuffer = nullptr;
    if (header->hasMorphs)
    {
        allMorphTargets = reinterpret_cast<const MeshFile_MorphTarget*>(fileData.data() + header->morphTargetDataOffset);
        allMorphDeltas = reinterpret_cast<const MeshFile_MorphDelta*>(fileData.data() + header->morphDeltaDataOffset);
        morphTargetNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->morphTargetNameOffset);
    }

    // Load ALL submeshes
    model.submeshes.reserve(header->numMeshes);
    for (uint32_t meshIdx = 0; meshIdx < header->numMeshes; ++meshIdx)
    {
        const MeshInfo& meshInfo = meshInfos[meshIdx];
        ModelSubmesh submesh;

        submesh.mesh.name = &meshNameBuffer[meshInfo.nameOffset];
        submesh.mesh.bounds = meshInfo.meshBounds;

        // Build material path from embedded name
        const char* materialName = &materialNamesBuffer[meshInfo.materialNameIndex];
        if (materialName[0] != '\0')
        {
            submesh.materialPath = materialsBasePath + materialName + ".material";
        }

        // Calculate vertex count
        uint32_t vertexCount = (meshIdx + 1 < header->numMeshes)
            ? (meshInfos[meshIdx + 1].firstVertex - meshInfo.firstVertex)
            : (header->totalVertices - meshInfo.firstVertex);

        // Extract vertices
        const Vertex* srcVertices = reinterpret_cast<const Vertex*>(allVertices + meshInfo.firstVertex);
        submesh.mesh.vertices.assign(srcVertices, srcVertices + vertexCount);

        // Extract indices (convert from global to local)
        submesh.mesh.indices.reserve(meshInfo.indexCount);
        for (uint32_t i = 0; i < meshInfo.indexCount; ++i)
        {
            uint32_t globalIndex = allIndices[meshInfo.firstIndex + i];
            uint32_t localIndex = globalIndex - meshInfo.firstVertex;
            submesh.mesh.indices.push_back(localIndex);
        }

        // Extract skinning data
        if (header->hasSkeleton)
        {
            submesh.mesh.skinning.assign(allSkinningData + meshInfo.firstVertex,
                                         allSkinningData + meshInfo.firstVertex + vertexCount);
            submesh.mesh.skeleton = model.skeleton;
        }

        // Extract morph targets
        if (header->hasMorphs && meshInfo.morphTargetCount > 0)
        {
            for (uint32_t t = 0; t < meshInfo.morphTargetCount; ++t)
            {
                const auto& fileTarget = allMorphTargets[meshInfo.firstMorphTarget + t];
                MorphTargetData target;
                target.name = &morphTargetNameBuffer[fileTarget.nameOffset];

                // Remap deltas to local indices
                for (uint32_t d = 0; d < fileTarget.deltaCount; ++d)
                {
                    const auto& fileDelta = allMorphDeltas[fileTarget.firstDelta + d];
                    if (fileDelta.vertexIndex >= meshInfo.firstVertex &&
                        fileDelta.vertexIndex < (meshInfo.firstVertex + vertexCount))
                    {
                        MorphTargetVertexDelta delta;
                        delta.vertexIndex = fileDelta.vertexIndex - meshInfo.firstVertex;
                        delta.deltaPosition = fileDelta.deltaPosition;
                        delta.deltaNormal = fileDelta.deltaNormal;
                        delta.deltaTangent = fileDelta.deltaTangent;
                        target.deltas.push_back(delta);
                    }
                }
                submesh.mesh.morphTargets.push_back(std::move(target));
            }
        }

        model.submeshes.push_back(std::move(submesh));
        LOG_DEBUG("  Submesh '{}' - {} verts, {} indices, material: {}",
                  model.submeshes.back().mesh.name,
                  model.submeshes.back().mesh.vertices.size(),
                  model.submeshes.back().mesh.indices.size(),
                  model.submeshes.back().materialPath);
    }

    LOG_INFO("Loaded model '{}' - {} submeshes", vfsPath, model.submeshes.size());
    return model;
}

MeshFileInfo AssetLoader::QueryMeshFileInfo(const std::string& vfsPath)
{
    MeshFileInfo info;
    info.sourcePath = vfsPath;

    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Mesh file not found: {}", vfsPath);
        return info;
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read mesh file failed: {}", vfsPath);
        return info;
    }

    if (fileData.size() < sizeof(MeshFileHeader))
    {
        return info;
    }

    const MeshFileHeader* header = reinterpret_cast<const MeshFileHeader*>(fileData.data());
    if (header->magic != MESH_FILE_MAGIC)
    {
        return info;
    }

    info.hasSkeleton = header->hasSkeleton;
    info.hasMorphs = header->hasMorphs;
    info.totalVertices = header->totalVertices;
    info.totalIndices = header->totalIndices;

    const MeshInfo* meshInfos = reinterpret_cast<const MeshInfo*>(fileData.data() + header->meshInfoDataOffset);
    const char* meshNameBuffer = reinterpret_cast<const char*>(fileData.data() + header->meshNamesOffset);
    const char* materialNamesBuffer = reinterpret_cast<const char*>(fileData.data() + header->materialNamesOffset);

    info.submeshes.reserve(header->numMeshes);
    for (uint32_t i = 0; i < header->numMeshes; ++i)
    {
        const MeshInfo& meshInfo = meshInfos[i];
        SubmeshInfo submesh;
        submesh.name = &meshNameBuffer[meshInfo.nameOffset];
        submesh.materialName = &materialNamesBuffer[meshInfo.materialNameIndex];
        submesh.bounds = meshInfo.meshBounds;
        submesh.indexCount = meshInfo.indexCount;

        // Calculate vertex count
        submesh.vertexCount = (i + 1 < header->numMeshes)
            ? (meshInfos[i + 1].firstVertex - meshInfo.firstVertex)
            : (header->totalVertices - meshInfo.firstVertex);

        info.submeshes.push_back(std::move(submesh));
    }

    return info;
}

// ============================================================================
// MATERIAL LOADING (.material)
// ============================================================================

ProcessedMaterial AssetLoader::LoadMaterial(const std::string& vfsPath)
{
    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Material file not found: {}", vfsPath);
        return ProcessedMaterial{};
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read material file failed: {}", vfsPath);
        return ProcessedMaterial{};
    }

    return ParseMaterialFile(fileData);
}

ProcessedMaterial AssetLoader::ParseMaterialFile(const std::vector<uint8_t>& fileData)
{
    ProcessedMaterial material{};

    // Parse JSON document
    rapidjson::Document doc;
    doc.Parse(reinterpret_cast<const char*>(fileData.data()), fileData.size());

    if (doc.HasParseError() || !doc.IsObject())
    {
        LOG_ERROR("Material file is not valid JSON");
        return material;
    }

    // Read name
    if (doc.HasMember("name") && doc["name"].IsString())
    {
        material.name = doc["name"].GetString();
    }

    // Read alpha mode
    if (doc.HasMember("alphaMode") && doc["alphaMode"].IsString())
    {
        std::string mode = doc["alphaMode"].GetString();
        if (mode == "Mask")
            material.alphaMode = AlphaMode::Mask;
        else if (mode == "Blend")
            material.alphaMode = AlphaMode::Blend;
        else
            material.alphaMode = AlphaMode::Opaque;
    }

    // Read scalar properties
    if (doc.HasMember("alphaCutoff") && doc["alphaCutoff"].IsNumber())
        material.alphaCutoff = doc["alphaCutoff"].GetFloat();

    if (doc.HasMember("metallicFactor") && doc["metallicFactor"].IsNumber())
        material.metallicFactor = doc["metallicFactor"].GetFloat();

    if (doc.HasMember("roughnessFactor") && doc["roughnessFactor"].IsNumber())
        material.roughnessFactor = doc["roughnessFactor"].GetFloat();

    if (doc.HasMember("normalScale") && doc["normalScale"].IsNumber())
        material.normalScale = doc["normalScale"].GetFloat();

    if (doc.HasMember("occlusionStrength") && doc["occlusionStrength"].IsNumber())
        material.occlusionStrength = doc["occlusionStrength"].GetFloat();

    // Read base color factor (array of 4 floats)
    if (doc.HasMember("baseColorFactor") && doc["baseColorFactor"].IsArray())
    {
        const auto& arr = doc["baseColorFactor"].GetArray();
        if (arr.Size() == 4)
        {
            material.baseColorFactor = vec4{
                arr[0].GetFloat(),
                arr[1].GetFloat(),
                arr[2].GetFloat(),
                arr[3].GetFloat()
            };
        }
    }

    // Read emissive factor (array of 3 floats)
    if (doc.HasMember("emissiveFactor") && doc["emissiveFactor"].IsArray())
    {
        const auto& arr = doc["emissiveFactor"].GetArray();
        if (arr.Size() == 3)
        {
            material.emissiveFactor = vec3{
                arr[0].GetFloat(),
                arr[1].GetFloat(),
                arr[2].GetFloat()
            };
        }
    }

    // Read texture paths - supports both array format (from asset compiler) and object format
    if (doc.HasMember("textures"))
    {
        if (doc["textures"].IsArray())
        {
            // Array format: [{"key": "baseColor", "value": "path"}, ...]
            const auto& texArr = doc["textures"].GetArray();
            for (const auto& entry : texArr)
            {
                if (!entry.IsObject() || !entry.HasMember("key") || !entry.HasMember("value"))
                    continue;

                std::string key = entry["key"].GetString();
                std::string value = entry["value"].GetString();

                if (value.empty())
                    continue;

                TextureDataSource source = FilePathSource{value};

                if (key == "baseColor")
                    material.baseColorTexture.source = source;
                else if (key == "metallicRoughness")
                    material.metallicRoughnessTexture.source = source;
                else if (key == "normal")
                    material.normalTexture.source = source;
                else if (key == "emissive")
                    material.emissiveTexture.source = source;
                else if (key == "occlusion")
                    material.occlusionTexture.source = source;
            }
        }
        else if (doc["textures"].IsObject())
        {
            // Object format: {"baseColor": "path", ...}
            const auto& texObj = doc["textures"].GetObj();

            auto loadTexturePath = [](const rapidjson::Value& obj, const char* key) -> TextureDataSource
            {
                if (obj.HasMember(key) && obj[key].IsString())
                {
                    std::string path = obj[key].GetString();
                    if (!path.empty())
                        return FilePathSource{path};
                }
                return std::monostate{};
            };

            material.baseColorTexture.source = loadTexturePath(texObj, "baseColor");
            material.metallicRoughnessTexture.source = loadTexturePath(texObj, "metallicRoughness");
            material.normalTexture.source = loadTexturePath(texObj, "normal");
            material.emissiveTexture.source = loadTexturePath(texObj, "emissive");
            material.occlusionTexture.source = loadTexturePath(texObj, "occlusion");
        }
    }

    // Read flags
    if (doc.HasMember("flags") && doc["flags"].IsUint())
    {
        material.flags = doc["flags"].GetUint();
    }

    // Set default flags
    material.materialTypeFlags = MaterialType::METALLIC_ROUGHNESS;
    material.flags |= MaterialFlags::DEFAULT_FLAGS;

    // Encode alpha mode into flags
    uint32_t alphaModeFlags = static_cast<uint32_t>(material.alphaMode) & MaterialFlags::ALPHA_MODE_MASK;
    material.flags = (material.flags & ~MaterialFlags::ALPHA_MODE_MASK) | alphaModeFlags;

    LOG_DEBUG("Loaded material '{}' - alphaMode: {}",
              material.name,
              material.alphaMode == AlphaMode::Opaque ? "Opaque" :
              material.alphaMode == AlphaMode::Mask ? "Mask" : "Blend");

    return material;
}

// ============================================================================
// ANIMATION LOADING (.anim)
// ============================================================================

ProcessedAnimationClip AssetLoader::LoadAnimation(const std::string& vfsPath)
{
    if (!VFS::FileExists(vfsPath))
    {
        LOG_WARNING("Animation file not found: {}", vfsPath);
        return ProcessedAnimationClip{};
    }

    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(vfsPath, fileData))
    {
        LOG_WARNING("VFS: Read animation file failed: {}", vfsPath);
        return ProcessedAnimationClip{};
    }

    return ParseAnimFile(fileData);
}

ProcessedAnimationClip AssetLoader::ParseAnimFile(const std::vector<uint8_t>& fileData)
{
    if (fileData.size() < sizeof(AnimationFileHeader))
    {
        LOG_ERROR("Animation file too small to contain header");
        return ProcessedAnimationClip{};
    }

    // Read header
    const AnimationFileHeader* header = reinterpret_cast<const AnimationFileHeader*>(fileData.data());

    if (header->magic != ANIM_FILE_MAGIC)
    {
        LOG_ERROR("Invalid animation file format (bad magic number)");
        return ProcessedAnimationClip{};
    }

    // Extract pointers to all data blocks
    const BoneAnimationChannel* boneChannels =
        reinterpret_cast<const BoneAnimationChannel*>(fileData.data() + header->channelDataOffset);
    const char* skeletalKeyframeBuffer =
        reinterpret_cast<const char*>(fileData.data() + header->keyframeDataOffset);
    const char* skeletalNameBuffer =
        reinterpret_cast<const char*>(fileData.data() + header->skeletalNameBufferOffset);

    const AnimationFile_MorphChannel* morphChannels =
        reinterpret_cast<const AnimationFile_MorphChannel*>(fileData.data() + header->morphChannelDataOffset);
    const char* morphKeyBuffer =
        reinterpret_cast<const char*>(fileData.data() + header->morphKeyDataOffset);
    const char* morphIndexBuffer =
        reinterpret_cast<const char*>(fileData.data() + header->morphIndexDataOffset);
    const char* morphWeightBuffer =
        reinterpret_cast<const char*>(fileData.data() + header->morphWeightDataOffset);

    // Create animation clip
    ProcessedAnimationClip clip;
    clip.duration = header->duration;
    clip.ticksPerSecond = header->ticksPerSecond;

    // --- Un-flatten Skeletal Channels ---
    clip.skeletalChannels.resize(header->numChannels);
    for (uint32_t i = 0; i < header->numChannels; ++i)
    {
        const auto& fileChannel = boneChannels[i];
        auto& engineChannel = clip.skeletalChannels[i];

        // Read joint name from name buffer
        engineChannel.nodeName = &skeletalNameBuffer[fileChannel.nameOffset];

        // Extract keyframes using offsets into binary blob
        const ProcessedAnimationVectorKey* posKeys =
            reinterpret_cast<const ProcessedAnimationVectorKey*>(
                &skeletalKeyframeBuffer[fileChannel.positionKeyOffset]);
        const ProcessedAnimationQuatKey* rotKeys =
            reinterpret_cast<const ProcessedAnimationQuatKey*>(
                &skeletalKeyframeBuffer[fileChannel.rotationKeyOffset]);
        const ProcessedAnimationVectorKey* scaleKeys =
            reinterpret_cast<const ProcessedAnimationVectorKey*>(
                &skeletalKeyframeBuffer[fileChannel.scaleKeyOffset]);

        // Copy keyframe data
        engineChannel.translationKeys.assign(posKeys, posKeys + fileChannel.numPositionKeys);
        engineChannel.rotationKeys.assign(rotKeys, rotKeys + fileChannel.numRotationKeys);
        engineChannel.scaleKeys.assign(scaleKeys, scaleKeys + fileChannel.numScaleKeys);
    }

    // --- Un-flatten Morph Channels ---
    clip.morphChannels.resize(header->numMorphChannels);
    for (uint32_t i = 0; i < header->numMorphChannels; ++i)
    {
        const auto& fileChannel = morphChannels[i];
        auto& engineChannel = clip.morphChannels[i];

        engineChannel.meshIndex = fileChannel.meshIndex;
        engineChannel.keys.resize(fileChannel.numKeys);

        // Find this channel's keys in the key blob
        const AnimationFile_MorphKey* fileKeys =
            reinterpret_cast<const AnimationFile_MorphKey*>(
                &morphKeyBuffer[fileChannel.keyOffset]);

        // Loop through each key to un-flatten its indices and weights
        for (uint32_t j = 0; j < fileChannel.numKeys; ++j)
        {
            const auto& fileKey = fileKeys[j];
            auto& engineKey = engineChannel.keys[j];

            engineKey.time = fileKey.time;

            // Find this key's indices and weights in their respective blobs
            const uint32_t* indices =
                reinterpret_cast<const uint32_t*>(
                    &morphIndexBuffer[fileKey.targetIndexOffset]);
            const float* weights =
                reinterpret_cast<const float*>(
                    &morphWeightBuffer[fileKey.weightOffset]);

            // Copy the data
            engineKey.targetIndices.assign(indices, indices + fileKey.numTargets);
            engineKey.weights.assign(weights, weights + fileKey.numTargets);
        }
    }

    LOG_DEBUG("Loaded animation clip - duration: {:.2f}s, {} skeletal channels, {} morph channels",
              clip.duration / clip.ticksPerSecond,
              clip.skeletalChannels.size(),
              clip.morphChannels.size());

    return clip;
}

} // namespace Resource
