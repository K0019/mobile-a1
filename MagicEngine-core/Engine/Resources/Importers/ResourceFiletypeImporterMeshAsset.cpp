#include "Engine/Resources/Importers/ResourceFiletypeImporterMeshAsset.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/ResourceManager.h"
#include "GameSettings.h"

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "tools/assets/io/import_config.h"

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
};
#pragma pack(pop)



namespace internal
{
    bool ImportMeshAsset(
        const std::string& filepath, 
        std::vector<MeshHandle>* outMeshHandles, std::vector<std::pair<uint32_t, Mat4>>* outMeshTransforms)
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
        std::vector<Vertex> allVertices(header.totalVertices); // Assuming Vertex struct

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

            nameOffsetToMaterialIndex[offset] = static_cast<uint32_t>(nameOffsetToMaterialIndex.size() - 1);
            p += name.length() + 1; // material names separated by null terminator
        }

        // Construct meshes from loaded vertices
        std::vector<Resource::ProcessedMesh> processedMeshes;
        processedMeshes.reserve(meshInfos.size());

        for (const auto& meshInfo : meshInfos)
        {
            Resource::ProcessedMesh mesh;

            // Find the material index for this mesh
            mesh.materialIndex = nameOffsetToMaterialIndex[meshInfo.materialNameIndex];

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

            processedMeshes.push_back(mesh);
        }

        // Upload to GPU 
        auto& graphicsAssetSystem{ ST<GraphicsMain>::Get()->GetAssetSystem() };


        outMeshHandles->reserve(processedMeshes.size());
        for (const auto& mesh : processedMeshes)
            outMeshHandles->push_back(graphicsAssetSystem.createMesh(mesh));

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

        graphicsAssetSystem.FlushUploads();
        return true;
    }

    void SetResourceHandlesMesh(const std::vector<AssociatedResourceHashes>& resourceHashes, 
        const std::vector<MeshHandle>& meshHandles, const std::vector<std::pair<uint32_t, Mat4>> meshTransforms /*, const std::vector<MaterialHandle>& materialHandles*/)
    {
        auto& meshes{ ST<MagicResourceManager>::Get()->INTERNAL_GetMeshes() };
        const auto& meshHashes{ resourceHashes[0].hashes };

        ResourceMesh* mesh{ meshes.INTERNAL_GetResource(meshHashes[0], true) };
        mesh->handles = meshHandles;
        mesh->transforms.resize(meshTransforms.size());
        for (const auto& [index, mat] : meshTransforms)
            mesh->transforms[index] = mat;
    }

}


bool ResourceFiletypeImporterMeshAsset::Import(const std::string& assetRelativeFilepath)
{
    // Load the meshes within the file
    std::vector<MeshHandle> meshHandles;
    std::vector<std::pair<uint32_t, Mat4>> meshTransforms;

    if (!internal::ImportMeshAsset(assetRelativeFilepath, &meshHandles, &meshTransforms))
        return false;

    // Create the file entry and resource hash
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMesh>(assetRelativeFilepath, 1) };

    // Set the meshes to the resource
    internal::SetResourceHandlesMesh(fileEntry->associatedResources, meshHandles, meshTransforms);

    return true;
}
