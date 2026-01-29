/******************************************************************************/
/*!
\file   ResourceImporters.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
Consolidated implementations for all resource import functions.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceImporters.h"
#include "Engine/Resources/Importers/ResourceImportHelpers.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Resources/ResourceImporter.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Managers/AudioManager.h"
#include "resource/resource_types.h"
#include "renderer/gfx_interface.h"
#include "tools/assets/io/import_config.h"
#include "FilepathConstants.h"
#include "GameSettings.h"
#include "VFS/VFS.h"
#include "core_utils/util.h"  // For SCOPE_EXIT
#include "resource/asset_formats/mesh_format.h"
#include "resource/asset_formats/anim_format.h"

// Use engine's file format types
using Resource::MESH_FILE_MAGIC;
using Resource::MeshFileHeader;
using Resource::MeshNode;
using Resource::MeshInfo;
using Resource::MeshFile_Bone;
using Resource::MeshFile_MorphDelta;
using Resource::MeshFile_MorphTarget;
using Resource::ANIM_FILE_MAGIC;
using Resource::AnimationFileHeader;
using Resource::BoneAnimationChannel;
using Resource::AnimationFile_MorphChannel;
using Resource::AnimationFile_MorphKey;

#ifdef GLFW
#include "tools/assets/io/texture_loader.h"
#include "tools/assets/io/material_loader.h"
#include "tools/assets/io/mesh_loader.h"
#include "Engine/Resources/AssetCompilerInterface.h"
#else
#include "ktx.h"
#endif

#include "Engine/Resources/MaterialSerialization.h"
#include "Game/Weapon.h"

using namespace ResourceImportHelpers;

// ============================================================================
// KTX Importer
// ============================================================================

#ifndef GLFW
namespace {
    Resource::ProcessedTexture ManualLoadTextureKTX(const std::string& filepath)
    {
        // This is hardcoded to make android work for now. For this, we're only supporting ktx2.
        assert(VFS::GetExtension(filepath) == ".ktx2");

        Resource::ProcessedTexture texture{};
        texture.source = FilePathSource{ filepath };
        texture.name = VFS::GetFilename(filepath);
        ktxTexture2* ktxTex = nullptr;

        std::vector<uint8_t> fileData;
        if (!VFS::ReadFile(filepath, fileData))
        {
            CONSOLE_LOG(LEVEL_ERROR) << "VFS: Read texture file failed: " << filepath;
            return texture;
        }

        KTX_error_code result = ktxTexture2_CreateFromMemory(fileData.data(), fileData.size(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

        if (result != KTX_SUCCESS || !ktxTex)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to load KTX2 file " << filepath << ".KTX Error : " << ktxErrorString(result);
            if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
            return texture;
        }

        if (ktxTex->classId != class_id::ktxTexture2_c)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "File " << filepath << "is not a KTX2 file. Found KTX1 instead.";
            ktxTexture_Destroy(ktxTexture(ktxTex));
            return texture;
        }

        SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

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
            .debugName = filepath.c_str() };

        CONSOLE_LOG(LEVEL_DEBUG) << "Loaded KTX2 texture " << filepath << " - " << texture.width << "x" << texture.height << ", " << ktxTex->numLevels << " mips";
        return texture;
    }
}
#endif

bool ResourceImporters::ImportKTX(const std::string& assetRelativeFilepath)
{
#ifdef GLFW
    // Load the file
    FilePathSource filepathSource{ assetRelativeFilepath };

    Resource::LoadingConfig config{ Resource::LoadingConfig::createBalanced() };
    Resource::ProcessedTexture processedTexture{ Resource::TextureLoading::extractTexture(filepathSource, config) };
#else
    Resource::ProcessedTexture processedTexture{ ManualLoadTextureKTX(assetRelativeFilepath) };
#endif
    if (processedTexture.data.empty())
        return false;

    // Create texture handle
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    TextureHandle textureHandle{ graphicsAssetSystem.createTexture(processedTexture) };

    // Create resource entry
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceTexture>(assetRelativeFilepath, 1) };

    // Assign resource to texture handle
    ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceTexture>().INTERNAL_GetResource(fileEntry->associatedResources[0].hashes[0], true)->handle = textureHandle;

    return true;
}

// ============================================================================
// Material Importer
// ============================================================================

namespace {
    void populateMaterialTexturesVector(Resource::ProcessedMaterial& material)
    {
        material.textures.clear();

        // Helper to add valid texture sources
        auto addIfValid = [&](const MaterialTexture& matTex)
            {
                if (matTex.hasTexture())
                {
                    material.textures.push_back(matTex.source);
                }
            };
        addIfValid(material.baseColorTexture);
        addIfValid(material.metallicRoughnessTexture);
        addIfValid(material.normalTexture);
        addIfValid(material.emissiveTexture);
        addIfValid(material.occlusionTexture);
    }
}

bool ResourceImporters::ImportMaterial(const std::string& relativeFilepath)
{
    // Load the file
    std::string fileData;
    if (!VFS::ReadFile(relativeFilepath, fileData))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open .material file ";
    }

    Resource::ProcessedMaterial material;

    Deserializer reader{ fileData };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize .material file ";
        return false;
    }
    MaterialSerialization::Deserialize(reader, material);
    populateMaterialTexturesVector(material);

    //Ensure that textures are loaded before we link them with the materials
    for (const auto& textureDataSource : material.textures)
    {
        if (const auto* filePath = std::get_if<FilePathSource>(&textureDataSource))
        {
            std::string virtualPath = filePath->path;
            std::transform(virtualPath.begin(), virtualPath.end(), virtualPath.begin(), ::tolower);
            ResourceImporter::Import(virtualPath);
        }
    }

    // Create material handle
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    MaterialHandle materialHandle{ graphicsAssetSystem.createMaterial(material) };

    // Create resource entry
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMaterial>(relativeFilepath, 1) };

    // Assign resource to material handle
    ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceMaterial>().INTERNAL_GetResource(fileEntry->associatedResources[0].hashes[0], true)->handle = materialHandle;

    return true;
}

// ============================================================================
// Audio Importer
// ============================================================================

bool ResourceImporters::ImportAudio(const std::string& assetRelativeFilepath)
{
    // Load the file into FMOD
    std::vector<uint8_t> fileData;
    if (!VFS::ReadFile(assetRelativeFilepath, fileData))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "VFS failed to read audio file: " << assetRelativeFilepath;
        return false;
    }

    auto sound{ ST<AudioManager>::Get()->CreateSoundFromData(reinterpret_cast<const char*>(fileData.data()), fileData.size()) };
    if (!sound)
        return false;

    // Create fileentry
    const ResourceFilepaths::FileEntry* fileentry{ ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath) };
    if (!fileentry)
    {
        static const size_t audioTypeHash = util::ConsistentHash<ResourceAudio>();  // Cache hash
        std::vector<AssociatedResourceHashes> resourceHashes{ 1 };
        resourceHashes[0].resourceTypeHash = audioTypeHash;
        resourceHashes[0].hashes.push_back(util::GenHash(VFS::GetStem(VFS::NormalizePath(assetRelativeFilepath))));
        GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
        fileentry = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes));
    }

    // Set the resource to the FMOD sound
    size_t hash{ fileentry->associatedResources[0].hashes[0] };
    auto* resource{ ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceAudio>().INTERNAL_GetResource(hash, true) };
    resource->sound = sound;
    // Note: Currently no metadata is set for sounds. Perhaps we can read a file associated with the audio here to load its metadata, similar to unity's metadata file method.

    return true;
}

// ============================================================================
// Mesh Asset Importer
// ============================================================================

namespace {
    std::string to_lower_str(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }

    bool ImportMeshAssetInternal(
        const std::string& filepath,
        std::vector<MeshHandle>* outMeshHandles,
        std::vector<std::pair<uint32_t, Mat4>>* outMeshTransforms,
        std::vector<size_t>* outMaterialHashes)
    {
        auto file = VFS::OpenFile(filepath, FileMode::Read);
        if (!file)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "VFS: Failed to open mesh file: " << filepath;
            return false;
        }

        //Validate Header
        MeshFileHeader header;
        file->Read(&header, sizeof(MeshFileHeader));

        if (header.magic != MESH_FILE_MAGIC)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid mesh asset file format: " << filepath;
            return false;
        }

        // Read file data
        std::vector<MeshNode> nodes(header.numNodes);
        std::vector<MeshInfo> meshInfos(header.numMeshes);
        std::vector<char> meshNameBuffer(header.meshNameBufferSize);
        std::vector<char> materialNamesBuffer(header.materialNameBufferSize);
        std::vector<uint32_t> allIndices(header.totalIndices);
        std::vector<Vertex> allVertices(header.totalVertices);

        std::vector<Resource::SkinningData> allSkinningData(header.totalVertices);
        std::vector<MeshFile_Bone> allBones(header.numBones);
        std::vector<char> boneNameBuffer(header.boneNameBufferSize);

        std::vector<MeshFile_MorphTarget> allMorphTargets(header.numMorphTargets);
        std::vector<MeshFile_MorphDelta> allMorphDeltas(header.numMorphDeltas);
        std::vector<char> morphTargetNameBuffer(header.morphTargetNameBufferSize);

        file->Seek(header.nodeDataOffset, SeekOrigin::Begin);
        file->Read(nodes.data(), nodes.size() * sizeof(MeshNode));

        file->Seek(header.meshInfoDataOffset, SeekOrigin::Begin);
        file->Read(meshInfos.data(), meshInfos.size() * sizeof(MeshInfo));

        file->Seek(header.meshNamesOffset, SeekOrigin::Begin);
        file->Read(meshNameBuffer.data(), meshNameBuffer.size());

        file->Seek(header.materialNamesOffset, SeekOrigin::Begin);
        file->Read(materialNamesBuffer.data(), materialNamesBuffer.size());

        file->Seek(header.indexDataOffset, SeekOrigin::Begin);
        file->Read(allIndices.data(), allIndices.size() * sizeof(uint32_t));

        file->Seek(header.vertexDataOffset, SeekOrigin::Begin);
        file->Read(allVertices.data(), allVertices.size() * sizeof(Vertex));

        if (header.hasSkeleton)
        {
            file->Seek(header.skinningDataOffset, SeekOrigin::Begin);
            file->Read(allSkinningData.data(), allSkinningData.size() * sizeof(Resource::SkinningData));

            file->Seek(header.boneDataOffset, SeekOrigin::Begin);
            file->Read(allBones.data(), allBones.size() * sizeof(MeshFile_Bone));

            file->Seek(header.boneNameOffset, SeekOrigin::Begin);
            file->Read(boneNameBuffer.data(), boneNameBuffer.size());
        }

        if (header.hasMorphs)
        {
            file->Seek(header.morphTargetDataOffset, SeekOrigin::Begin);
            file->Read(allMorphTargets.data(), allMorphTargets.size() * sizeof(MeshFile_MorphTarget));

            file->Seek(header.morphDeltaDataOffset, SeekOrigin::Begin);
            file->Read(allMorphDeltas.data(), allMorphDeltas.size() * sizeof(MeshFile_MorphDelta));

            file->Seek(header.morphTargetNameOffset, SeekOrigin::Begin);
            file->Read(morphTargetNameBuffer.data(), morphTargetNameBuffer.size());
        }

        // Retrieve material names for meshes
        std::map<uint32_t, uint32_t> nameOffsetToMaterialIndex;

        const char* p = materialNamesBuffer.data();
        while (p < materialNamesBuffer.data() + materialNamesBuffer.size())
        {
            uint32_t offset = static_cast<uint32_t>(p - materialNamesBuffer.data());
            std::string name(p);

            if (name.empty()) break;

            nameOffsetToMaterialIndex[offset] = static_cast<uint32_t>(nameOffsetToMaterialIndex.size());

            p += name.length() + 1;
        }

        // Rebuild the skeleton
        Resource::ProcessedSkeleton tempSkeleton;
        if (header.hasSkeleton)
        {
            tempSkeleton.parentIndices.resize(header.numBones);
            tempSkeleton.inverseBindMatrices.resize(header.numBones);
            tempSkeleton.bindPoseMatrices.resize(header.numBones);
            tempSkeleton.jointNames.resize(header.numBones);

            for (uint32_t i = 0; i < header.numBones; ++i)
            {
                const auto& fileBone = allBones[i];

                tempSkeleton.parentIndices[i] = fileBone.parentIndex;
                tempSkeleton.inverseBindMatrices[i] = fileBone.inverseBindPose;
                tempSkeleton.bindPoseMatrices[i] = fileBone.bindPose;
                tempSkeleton.jointNames[i] = &boneNameBuffer[fileBone.nameOffset];
            }
        }

        // Rebuild the morph target
        std::vector<Resource::MorphTargetData> tempMorphTargets;
        if (header.hasMorphs)
        {
            tempMorphTargets.resize(header.numMorphTargets);
            for (uint32_t i = 0; i < header.numMorphTargets; ++i)
            {
                const auto& fileTarget = allMorphTargets[i];
                auto& engineTarget = tempMorphTargets[i];

                engineTarget.name = &morphTargetNameBuffer[fileTarget.nameOffset];
                engineTarget.deltas.resize(fileTarget.deltaCount);

                for (uint32_t j = 0; j < fileTarget.deltaCount; ++j)
                {
                    const auto& fileDelta = allMorphDeltas[fileTarget.firstDelta + j];
                    auto& engineDelta = engineTarget.deltas[j];

                    engineDelta.vertexIndex = fileDelta.vertexIndex;
                    engineDelta.deltaPosition = fileDelta.deltaPosition;
                    engineDelta.deltaNormal = fileDelta.deltaNormal;
                    engineDelta.deltaTangent = fileDelta.deltaTangent;
                }
            }
        }

        // Construct meshes from loaded vertices
        std::vector<Resource::ProcessedMesh> processedMeshes;
        processedMeshes.reserve(meshInfos.size());

        for (const auto& meshInfo : meshInfos)
        {
            Resource::ProcessedMesh mesh;
            mesh.name = &meshNameBuffer[meshInfo.nameOffset];

            // Find the material index for this mesh
            mesh.materialIndex = nameOffsetToMaterialIndex[meshInfo.materialNameIndex];
            std::string materialname = { materialNamesBuffer.data() + meshInfo.materialNameIndex };
            to_lower_str(materialname);

            std::string constructedPathToMaterial = VFS::JoinPath(VFS::GetParentPath(filepath), materialname + ".material");
            size_t materialHash{ 0 };
            const auto& fpManager = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager();
            auto materialEntry = fpManager.GetFileEntry(constructedPathToMaterial);
            if (!materialEntry)
            {
                // Fallback: check materials/ subdirectory (for flat-copied meshes)
                std::string fallbackPath = VFS::JoinPath("compiledassets/materials", materialname + ".material");
                materialEntry = fpManager.GetFileEntry(fallbackPath);
            }
            if (materialEntry)
            {
                materialHash = materialEntry->associatedResources[0].hashes[0];
            }
            outMaterialHashes->push_back(materialHash);

            // Get the vertice and indice data
            uint32_t vertexCount = 0;
            if (&meshInfo != &meshInfos.back())
            {
                vertexCount = (&meshInfo + 1)->firstVertex - meshInfo.firstVertex;
            }
            else
            {
                vertexCount = static_cast<uint32_t>(allVertices.size()) - meshInfo.firstVertex;
            }
            mesh.vertices.assign(allVertices.begin() + meshInfo.firstVertex, allVertices.begin() + meshInfo.firstVertex + vertexCount);

            mesh.indices.reserve(meshInfo.indexCount);
            for (uint32_t i = 0; i < meshInfo.indexCount; ++i)
            {
                uint32_t originalIndex = allIndices[meshInfo.firstIndex + i] - meshInfo.firstVertex;
                mesh.indices.push_back(originalIndex);
            }

            mesh.bounds = meshInfo.meshBounds;

            if (header.hasSkeleton)
            {
                mesh.skinning.assign(allSkinningData.begin() + meshInfo.firstVertex, allSkinningData.begin() + meshInfo.firstVertex + vertexCount);
                mesh.skeleton = tempSkeleton;
            }

            if (header.hasMorphs && meshInfo.morphTargetCount > 0)
            {
                auto start = tempMorphTargets.begin() + meshInfo.firstMorphTarget;
                auto end = start + meshInfo.morphTargetCount;
                mesh.morphTargets.assign(start, end);

                for (auto& target : mesh.morphTargets)
                {
                    std::vector<Resource::MorphTargetVertexDelta> localDeltas;
                    for (const auto& delta : target.deltas)
                    {
                        if (delta.vertexIndex >= meshInfo.firstVertex && delta.vertexIndex < (meshInfo.firstVertex + vertexCount))
                        {
                            Resource::MorphTargetVertexDelta localDelta = delta;
                            localDelta.vertexIndex = delta.vertexIndex - meshInfo.firstVertex;
                            localDeltas.push_back(localDelta);
                        }
                    }
                    target.deltas = std::move(localDeltas);
                }
            }
            processedMeshes.push_back(mesh);
        }

        // Precompute world transforms
        std::vector<Mat4> worldTransforms(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].parentIndex == -1)
            {
                worldTransforms[i] = nodes[i].transform;
            }
            else
            {
                worldTransforms[i] = worldTransforms[nodes[i].parentIndex] * nodes[i].transform;
            }
        }

        outMeshTransforms->reserve(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].meshIndex != -1)
            {
                outMeshTransforms->push_back({ (uint32_t)nodes[i].meshIndex, worldTransforms[i] });
            }
        }

        // Scale bounds by node transform
        for (const auto& [index, mat] : *outMeshTransforms)
        {
            Resource::ProcessedMesh& mesh = processedMeshes[index];

            float scaleX = glm::length(vec3(mat[0]));
            float scaleY = glm::length(vec3(mat[1]));
            float scaleZ = glm::length(vec3(mat[2]));
            float nodeMaxScale = glm::max(scaleX, glm::max(scaleY, scaleZ));

            mesh.bounds.w = mesh.bounds.w * nodeMaxScale;
        }

        // Upload to GPU
        auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };

        outMeshHandles->reserve(processedMeshes.size());
        for (const auto& mesh : processedMeshes)
            outMeshHandles->push_back(graphicsAssetSystem.createMesh(mesh));

        return true;
    }

    void SetResourceHandlesMesh(const std::vector<AssociatedResourceHashes>& resourceHashes,
        const std::vector<MeshHandle>& meshHandles, const std::vector<std::pair<uint32_t, Mat4>> meshTransforms, const std::vector<size_t>& materialHashes)
    {
        auto& meshes{ ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceMesh>() };
        const auto& meshHashes{ resourceHashes[0].hashes };

        ResourceMesh* mesh{ meshes.INTERNAL_GetResource(meshHashes[0], true) };
        mesh->handles = meshHandles;
        mesh->transforms.resize(meshTransforms.size());

        for (const auto& [index, mat] : meshTransforms)
            mesh->transforms[index] = mat;

        mesh->defaultMaterialHashes = materialHashes;
    }
}

bool ResourceImporters::ImportMeshAsset(const std::string& assetRelativeFilepath)
{
    // Load the meshes within the file
    std::vector<MeshHandle> meshHandles;
    std::vector<std::pair<uint32_t, Mat4>> meshTransforms;
    std::vector<size_t> materialHashes;

    if (!ImportMeshAssetInternal(assetRelativeFilepath, &meshHandles, &meshTransforms, &materialHashes))
        return false;

    // Create the file entry and resource hash
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMesh>(assetRelativeFilepath, 1) };

    // Set the meshes to the resource
    SetResourceHandlesMesh(fileEntry->associatedResources, meshHandles, meshTransforms, materialHashes);

    return true;
}

// ============================================================================
// Animation Asset Importer
// ============================================================================

namespace {
    bool ImportAnimationAssetInternal(const std::string& filepath, Resource::ProcessedAnimationClip& outClip)
    {
        auto file = VFS::OpenFile(filepath, FileMode::Read);
        if (!file)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "VFS: Failed to open animation file: " << filepath;
            return false;
        }

        //Validate Header
        AnimationFileHeader header;
        file->Read(&header, sizeof(AnimationFileHeader));

        if (header.magic != ANIM_FILE_MAGIC)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid anim asset file format: " << filepath;
            return false;
        }

        // Read file data
        // --- Skeletal Blobs ---
        std::vector<BoneAnimationChannel> boneChannels(header.numChannels);
        file->Seek(header.channelDataOffset, SeekOrigin::Begin);
        file->Read(boneChannels.data(), header.numChannels * sizeof(BoneAnimationChannel));

        size_t skeletalKeyframeBlobSize = header.skeletalNameBufferOffset - header.keyframeDataOffset;
        std::vector<char> skeletalKeyframeBuffer(skeletalKeyframeBlobSize);
        file->Seek(header.keyframeDataOffset, SeekOrigin::Begin);
        file->Read(skeletalKeyframeBuffer.data(), skeletalKeyframeBlobSize);

        std::vector<char> skeletalNameBuffer(header.skeletalNameBufferSize);
        file->Seek(header.skeletalNameBufferOffset, SeekOrigin::Begin);
        file->Read(skeletalNameBuffer.data(), header.skeletalNameBufferSize);

        // --- Morph Blobs ---
        std::vector<AnimationFile_MorphChannel> morphChannels(header.numMorphChannels);
        file->Seek(header.morphChannelDataOffset, SeekOrigin::Begin);
        file->Read(morphChannels.data(), header.numMorphChannels * sizeof(AnimationFile_MorphChannel));

        size_t morphKeyBlobSize = header.morphIndexDataOffset - header.morphKeyDataOffset;
        std::vector<char> morphKeyBuffer(morphKeyBlobSize);
        file->Seek(header.morphKeyDataOffset, SeekOrigin::Begin);
        file->Read(morphKeyBuffer.data(), morphKeyBlobSize);

        size_t morphIndexBlobSize = header.morphWeightDataOffset - header.morphIndexDataOffset;
        std::vector<char> morphIndexBuffer(morphIndexBlobSize);
        file->Seek(header.morphIndexDataOffset, SeekOrigin::Begin);
        file->Read(morphIndexBuffer.data(), morphIndexBlobSize);

        size_t morphWeightBlobSize = header.morphWeightDataBufferSize;
        std::vector<char> morphWeightBuffer(morphWeightBlobSize);
        file->Seek(header.morphWeightDataOffset, SeekOrigin::Begin);
        file->Read(morphWeightBuffer.data(), morphWeightBlobSize);

        // Create the animation clip
        outClip.name = std::filesystem::path(filepath).stem().string();
        outClip.duration = header.duration;
        outClip.ticksPerSecond = header.ticksPerSecond;

        // --- Un-flatten Skeletal Channels ---
        outClip.skeletalChannels.resize(header.numChannels);
        for (uint32_t i = 0; i < header.numChannels; ++i)
        {
            const auto& fileChannel = boneChannels[i];
            auto& engineChannel = outClip.skeletalChannels[i];

            engineChannel.nodeName = &skeletalNameBuffer[fileChannel.nameOffset];

            const Resource::ProcessedAnimationVectorKey* posKeys = reinterpret_cast<const Resource::ProcessedAnimationVectorKey*>(&skeletalKeyframeBuffer[fileChannel.positionKeyOffset]);
            const Resource::ProcessedAnimationQuatKey* rotKeys = reinterpret_cast<const Resource::ProcessedAnimationQuatKey*>(&skeletalKeyframeBuffer[fileChannel.rotationKeyOffset]);
            const Resource::ProcessedAnimationVectorKey* scaleKeys = reinterpret_cast<const Resource::ProcessedAnimationVectorKey*>(&skeletalKeyframeBuffer[fileChannel.scaleKeyOffset]);

            engineChannel.translationKeys.assign(posKeys, posKeys + fileChannel.numPositionKeys);
            engineChannel.rotationKeys.assign(rotKeys, rotKeys + fileChannel.numRotationKeys);
            engineChannel.scaleKeys.assign(scaleKeys, scaleKeys + fileChannel.numScaleKeys);
        }

        // --- Un-flatten Morph Channels ---
        outClip.morphChannels.resize(header.numMorphChannels);
        for (uint32_t i = 0; i < header.numMorphChannels; ++i)
        {
            const auto& fileChannel = morphChannels[i];
            auto& engineChannel = outClip.morphChannels[i];

            engineChannel.meshIndex = fileChannel.meshIndex;
            engineChannel.keys.resize(fileChannel.numKeys);

            const AnimationFile_MorphKey* fileKeys = reinterpret_cast<const AnimationFile_MorphKey*>(&morphKeyBuffer[fileChannel.keyOffset]);

            for (uint32_t j = 0; j < fileChannel.numKeys; ++j)
            {
                const auto& fileKey = fileKeys[j];
                auto& engineKey = engineChannel.keys[j];

                engineKey.time = fileKey.time;

                const uint32_t* indices = reinterpret_cast<const uint32_t*>(&morphIndexBuffer[fileKey.targetIndexOffset]);
                const float* weights = reinterpret_cast<const float*>(&morphWeightBuffer[fileKey.weightOffset]);

                engineKey.targetIndices.assign(indices, indices + fileKey.numTargets);
                engineKey.weights.assign(weights, weights + fileKey.numTargets);
            }
        }

        return true;
    }

    void SetResourceHandlesAnimation(const std::vector<AssociatedResourceHashes>& resourceHashes, Resource::ClipId clipId)
    {
        auto& anims{ ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceAnimation>() };
        const auto& animHashes{ resourceHashes[0].hashes };

        ResourceAnimation* anim = { anims.INTERNAL_GetResource(animHashes[0], true) };
        anim->handle = clipId;
    }
}

bool ResourceImporters::ImportAnimationAsset(const std::string& assetRelativeFilepath)
{
    Resource::ProcessedAnimationClip clipToLoad;

    if (!ImportAnimationAssetInternal(assetRelativeFilepath, clipToLoad))
    {
        return false;
    }

    // Upload to GPU
    auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };
    Resource::ClipId clipId = graphicsAssetSystem.createClip(clipToLoad);

    if (clipId == Resource::INVALID_CLIP_ID)
    {
        return false;
    }

    const auto* fileEntry{ GenerateFileEntryForResources<ResourceAnimation>(assetRelativeFilepath, 1) };
    SetResourceHandlesAnimation(fileEntry->associatedResources, clipId);

    return true;
}

// ============================================================================
// FBX Importer (compilation delegate)
// ============================================================================

bool ResourceImporters::ImportFBX([[maybe_unused]] const std::string& assetRelativeFilepath)
{
#ifdef GLFW
    CompileAndImportAsset(assetRelativeFilepath);
#else
    CONSOLE_LOG_UNIMPLEMENTED() << "Importing FBX files is not implemented for this platform.";
    return false;
#endif
    return false;
}

// ============================================================================
// Image Importer (compilation delegate)
// ============================================================================

bool ResourceImporters::ImportImage([[maybe_unused]] const std::string& assetRelativeFilepath)
{
#ifdef GLFW
    CompileAndImportAsset(assetRelativeFilepath);
#else
    CONSOLE_LOG_UNIMPLEMENTED() << "Importing images is not implemented for this platform.";
    return false;
#endif
    return false;
}

// ============================================================================
// Game Weapon Importer
// ============================================================================

bool ResourceImporters::ImportGameWeapon(const std::string& assetRelativeFilepath)
{
    WeaponInfo weaponInfo{};
    Deserializer reader{ assetRelativeFilepath };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Unable to read game weapon file: " << assetRelativeFilepath;
        return false;
    }
    reader.Deserialize(&weaponInfo);

    const auto fileEntry{ GenerateFileEntryForResources<WeaponInfo>(assetRelativeFilepath, 1) };
    weaponInfo.hash = fileEntry->associatedResources[0].hashes[0];
    *ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<WeaponInfo>().INTERNAL_GetResource(weaponInfo.hash, true) = std::move(weaponInfo);
    return true;
}
