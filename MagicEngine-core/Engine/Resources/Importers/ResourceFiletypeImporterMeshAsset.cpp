#include "Engine/Resources/Importers/ResourceFiletypeImporterMeshAsset.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/ResourceManager.h"
#include "GameSettings.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "tools/assets/io/import_config.h"

#pragma region Mesh File Structure
// Just copy pasting definitions from assetcompiler/MeshFileStructure.h 
// To make engine not depend on assetcompiler project

constexpr uint32_t MESH_FILE_MAGIC = { 'MESH' };

#pragma pack(push, 1)
// Header at the very start of the file
struct MeshFileHeader
{
    uint32_t magic = MESH_FILE_MAGIC;

    uint32_t numNodes;
    uint32_t numMeshes;
    uint32_t totalIndices;
    uint32_t totalVertices;
    uint32_t materialNameBufferSize; // Total size of the material name block

    // Skelaton
    uint32_t hasSkeleton; // 1 or 0
    uint32_t numBones;
    uint32_t boneNameBufferSize;

    // Morphs
    uint32_t hasMorphs;
    uint32_t numMorphTargets;
    uint32_t numMorphDeltas;
    uint32_t morphTargetNameBufferSize;

    // Bounds for the entire scene.
    // This is actually not needed, since
    // in the mesh importer's SetResourceHandlesMesh, we take the bounds of every individual mesh,
    // and nowhere do we ever use the bounds of the entire scene.
    // might be useful to quickly cull the entire scene instead of part by part though??
    vec3 sceneBoundsCenter;
    float sceneBoundsRadius;
    vec3 sceneBoundsMin;
    vec3 sceneBoundsMax;

    // Offsets to the start of each data block from the beginning of the file
    uint64_t nodeDataOffset;
    uint64_t meshInfoDataOffset;
    uint64_t materialNamesOffset;
    uint64_t indexDataOffset;
    uint64_t vertexDataOffset;

    uint64_t skinningDataOffset;
    uint64_t boneDataOffset;
    uint64_t boneNameOffset;

    uint64_t morphTargetDataOffset;
    uint64_t morphDeltaDataOffset;
    uint64_t morphTargetNameOffset;
};

// Information for each node inside fbx
struct MeshNode
{
    mat4 transform;
    int32_t parentIndex; // Index into the node array, -1 for root
    int32_t meshIndex;   // Index into the mesh info array, -1 if no mesh
    char name[64];       // Fixed-size name for simplicity
};

// Describes how to get mesh data from the buffers
struct MeshInfo
{
    uint32_t indexCount;
    uint32_t firstIndex;        // Offset into the index buffer
    uint32_t firstVertex;       // Offset into the vertex buffer
    uint32_t materialNameIndex; // Index into the material name offset table

    // Bounding volume for this individual mesh part
    vec4 meshBounds; // (x,y,z, radius)

    uint32_t firstMorphTarget;  // Index into the global morphTargetData blob
    uint32_t morphTargetCount;  // Number of morph targets for this mesh
};

// Structure of bone data
struct MeshFile_Bone
{
    mat4     inverseBindPose; // Assimp's aiBone::mOffsetMatrix
    mat4     bindPose;        // The bone's global transform in bind pose
    int32_t  parentIndex;     // Index into this file's array of Bones. -1 for root.
    uint32_t nameOffset;      // Offset into the boneNameBuffer
};

// This struct holds the per-vertex changes
struct MeshFile_MorphDelta  //MorphTargetVertexDelta
{
    uint32_t vertexIndex;
    vec3     deltaPosition;
    vec3     deltaNormal;
    vec3     deltaTangent;
};

// This defines one morph target
struct MeshFile_MorphTarget //MorphTargetData
{
    uint32_t nameOffset;   // Offset into the morphTargetNameBuffer
    uint32_t firstDelta;   // Index into the main morphDeltaData blob
    uint32_t deltaCount;   // Number of deltas for this target
};

#pragma pack(pop)

#pragma endregion




namespace
{
    std::string to_lower(std::string& s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c) { return std::tolower(c); });
        return s;
    }
}

namespace internal
{
    bool ImportMeshAsset(
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
            //file.close();
            return false;
        }
        
        // Read file data
        std::vector<MeshNode> nodes(header.numNodes);
        std::vector<MeshInfo> meshInfos(header.numMeshes);
        std::vector<char> materialNamesBuffer(header.materialNameBufferSize);
        std::vector<uint32_t> allIndices(header.totalIndices);
        std::vector<Vertex> allVertices(header.totalVertices); // CompilerTypes.h's Vertex must be exactly the same
        
        std::vector<Resource::SkinningData> allSkinningData(header.totalVertices);  // So must this
        std::vector<MeshFile_Bone> allBones(header.numBones);
        std::vector<char> boneNameBuffer(header.boneNameBufferSize);

        std::vector<MeshFile_MorphTarget> allMorphTargets(header.numMorphTargets);
        std::vector<MeshFile_MorphDelta> allMorphDeltas(header.numMorphDeltas);
        std::vector<char> morphTargetNameBuffer(header.morphTargetNameBufferSize);

        file->Seek(header.nodeDataOffset, SeekOrigin::Begin);
        file->Read(nodes.data(), nodes.size() * sizeof(MeshNode));

        file->Seek(header.meshInfoDataOffset, SeekOrigin::Begin);
        file->Read(meshInfos.data(), meshInfos.size() * sizeof(MeshInfo));

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
        std::map<uint32_t, uint32_t> nameOffsetToMaterialIndex; // Map offset to final index
        // To retrive material name, 
        // materialNamesBuffer[nameOffsetToMaterialIndex[meshInfo.materialNameIndex]] and cast it to a string.
        // The material names are separated by \0

        const char* p = materialNamesBuffer.data();
        while (p < materialNamesBuffer.data() + materialNamesBuffer.size())
        {
            uint32_t offset = static_cast<uint32_t>(p - materialNamesBuffer.data());
            std::string name(p);

            if (name.empty()) break;

            nameOffsetToMaterialIndex[offset] = static_cast<uint32_t>(nameOffsetToMaterialIndex.size());

            p += name.length() + 1; // material names separated by null terminator
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

                    engineDelta.vertexIndex   = fileDelta.vertexIndex;
                    engineDelta.deltaPosition = fileDelta.deltaPosition;
                    engineDelta.deltaNormal   = fileDelta.deltaNormal;
                    engineDelta.deltaTangent  = fileDelta.deltaTangent;
                }
            }
        }

        // Construct meshes from loaded vertices
        std::vector<Resource::ProcessedMesh> processedMeshes;
        processedMeshes.reserve(meshInfos.size());

        for (const auto& meshInfo : meshInfos)
        {
            Resource::ProcessedMesh mesh;

            // Find the material index for this mesh
            mesh.materialIndex = nameOffsetToMaterialIndex[meshInfo.materialNameIndex];
            std::string materialname = { materialNamesBuffer.data() + meshInfo.materialNameIndex };
            to_lower(materialname);

            std::string constructedPathToMaterial = "compiledassets/materials/" + materialname + ".material";
            size_t materialHash { 0 };
            const auto& fpManager = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager();
            auto materialEntry = fpManager.GetFileEntry(constructedPathToMaterial);
            if (materialEntry)
            {
                //Assume only one associated resource, the material
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
                uint32_t originalIndex = allIndices[meshInfo.firstIndex + i] - meshInfo.firstVertex; // need to minus offset from global buffer to get actual indices to use
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
                // Assign this mesh its specific "slice" of the master list
                auto start = tempMorphTargets.begin() + meshInfo.firstMorphTarget;
                auto end = start + meshInfo.morphTargetCount;
                mesh.morphTargets.assign(start, end);

                // Remap the vertex indices in these deltas to be local to this mesh
                for (auto& target : mesh.morphTargets)
                {
                    std::vector<Resource::MorphTargetVertexDelta> localDeltas;
                    for (const auto& delta : target.deltas)
                    {
                        // Check if this delta's GLOBAL index belongs to THIS mesh
                        // Convert from global indices back to local indices
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

        // We could precomute this...
        std::vector<Mat4> worldTransforms(nodes.size());
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].parentIndex == -1)
            {
                worldTransforms[i] = nodes[i].transform; // It's a root node
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


        //Scale bounds by node transform
        std::map<uint32_t, Mat4> meshIndexToTransform;
        for (const auto& [index, mat] : *outMeshTransforms)
        {
            Resource::ProcessedMesh& mesh = processedMeshes[index];

            float scaleX = glm::length(vec3(mat[0]));
            float scaleY = glm::length(vec3(mat[1]));
            float scaleZ = glm::length(vec3(mat[2]));
            float nodeMaxScale = glm::max(scaleX, glm::max(scaleY, scaleZ));

            // Scale local radius
            // The bounds.center is currently already being handled by the this line in scene_feature.cpp
                    //auto worldCenter = vec3(obj.transform * vec4(center, 1.0f));
            // The radius of the bounds is not being handled yet, so we do it here
            
            mesh.bounds.w = mesh.bounds.w * nodeMaxScale;
        }

        // Upload to GPU 
        auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };

        outMeshHandles->reserve(processedMeshes.size());
        for (const auto& mesh : processedMeshes)
            outMeshHandles->push_back(graphicsAssetSystem.createMesh(mesh));



        graphicsAssetSystem.FlushUploads();
        return true;
    }

    void SetResourceHandlesMesh(const std::vector<AssociatedResourceHashes>& resourceHashes, 
        const std::vector<MeshHandle>& meshHandles, const std::vector<std::pair<uint32_t, Mat4>> meshTransforms, const std::vector<size_t>& materialHashes)
    {
        auto& meshes{ ST<MagicResourceManager>::Get()->INTERNAL_GetMeshes() };
        const auto& meshHashes{ resourceHashes[0].hashes };

        ResourceMesh* mesh{ meshes.INTERNAL_GetResource(meshHashes[0], true) };
        mesh->handles = meshHandles;
        mesh->transforms.resize(meshTransforms.size());

        for (const auto& [index, mat] : meshTransforms)
            mesh->transforms[index] = mat;
        
        mesh->defaultMaterialHashes = materialHashes;
    }

}


bool ResourceFiletypeImporterMeshAsset::Import(const std::string& assetRelativeFilepath)
{
    // Load the meshes within the file
    std::vector<MeshHandle> meshHandles;
    std::vector<std::pair<uint32_t, Mat4>> meshTransforms;
    std::vector<size_t> materialHashes;

    if (!internal::ImportMeshAsset(assetRelativeFilepath, &meshHandles, &meshTransforms, &materialHashes))
        return false;

    // Create the file entry and resource hash
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMesh>(assetRelativeFilepath, 1) };

    // Set the meshes to the resource
    internal::SetResourceHandlesMesh(fileEntry->associatedResources, meshHandles, meshTransforms, materialHashes);

    return true;
}
