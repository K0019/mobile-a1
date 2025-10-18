#include "Engine/Resources/Importers/ResourceFiletypeImporterMeshAsset.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/ResourceManager.h"
#include "GameSettings.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "tools/assets/io/import_config.h"
#include "MeshFileStructure.h"

namespace internal
{
    bool ImportMeshAsset(
        const std::filesystem::path& filepath, 
        std::vector<MeshHandle>* outMeshHandles, std::vector<std::pair<uint32_t, Mat4>>* outMeshTransforms)
    {

        std::ifstream file(filepath, std::ios::binary);
        if (!file.is_open())
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to import scene file: " << filepath.string();
            return false;
        }

        //Validate Header
        compiler::MeshFileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(compiler::MeshFileHeader));

        if (header.magic != compiler::MESH_FILE_MAGIC)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Invalid mesh asset file format: " << filepath.string();
            file.close();
            return false;
        }
        
        // Read file data
        std::vector<compiler::MeshNode> nodes(header.numNodes);
        std::vector<compiler::MeshInfo> meshInfos(header.numMeshes);
        std::vector<char> materialNamesBuffer(header.materialNameBufferSize);
        std::vector<uint32_t> allIndices(header.totalIndices);
        std::vector<Vertex> allVertices(header.totalVertices); // Assuming Vertex struct

        file.seekg(header.nodeDataOffset);
        file.read(reinterpret_cast<char*>(nodes.data()), nodes.size() * sizeof(compiler::MeshNode));

        file.seekg(header.meshInfoDataOffset);
        file.read(reinterpret_cast<char*>(meshInfos.data()), meshInfos.size() * sizeof(compiler::MeshInfo));

        file.seekg(header.materialNamesOffset);
        file.read(materialNamesBuffer.data(), materialNamesBuffer.size());

        file.seekg(header.indexDataOffset);
        file.read(reinterpret_cast<char*>(allIndices.data()), allIndices.size() * sizeof(uint32_t));

        file.seekg(header.vertexDataOffset);
        file.read(reinterpret_cast<char*>(allVertices.data()), allVertices.size() * sizeof(Vertex));

        file.close();

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
        auto graphicsAssetSystem{ ST<GraphicsAssets>::Get()->INTERNAL_GetAssetSystem() };


        outMeshHandles->reserve(processedMeshes.size());
        for (const auto& mesh : processedMeshes)
            outMeshHandles->push_back(graphicsAssetSystem->createMesh(mesh));

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

        graphicsAssetSystem->FlushUploads();
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


bool ResourceFiletypeImporterMeshAsset::Import(const std::filesystem::path& assetRelativeFilepath)
{
    // Load the meshes within the file
    std::vector<MeshHandle> meshHandles;
    std::vector<std::pair<uint32_t, Mat4>> meshTransforms;

    if (!internal::ImportMeshAsset(GetExeRelativeFilepath(assetRelativeFilepath), &meshHandles, &meshTransforms))
        return false;

    // Create the file entry and resource hash
    const auto* fileEntry{ GenerateFileEntryForResources<ResourceMesh>(assetRelativeFilepath, 1) };

    // Set the meshes to the resource
    internal::SetResourceHandlesMesh(fileEntry->associatedResources, meshHandles, meshTransforms);

    return true;
}
