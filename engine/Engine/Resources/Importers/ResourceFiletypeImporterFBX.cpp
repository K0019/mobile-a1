#include "ResourceFiletypeImporterFBX.h"
#include "ResourceTypesGraphics.h"
#include "ResourceManager.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "import_config.h"
#include "material_loader.h"
#include "texture_loader.h"
#include "mesh_loader.h"
#include "GraphicsAPI.h"

namespace internal {

    std::unique_ptr<const aiScene, void(*)(const aiScene*)> ImportScene(const std::filesystem::path& path)
    {
        if (!exists(path))
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Scene file not found: " << path.string();
            return { nullptr, [](const aiScene*) {} };
        }

        // Use a thread_local importer to ensure thread safety with Assimp
        thread_local Assimp::Importer importer;

        // Configure importer properties
        importer.SetPropertyInteger(AI_CONFIG_PP_SLM_VERTEX_LIMIT, 1'000'000);
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
        importer.SetPropertyFloat(AI_CONFIG_PP_GSN_MAX_SMOOTHING_ANGLE, 80.0f);

        const uint32_t flags = aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_GenSmoothNormals | aiProcess_ImproveCacheLocality | aiProcess_ValidateDataStructure;

        const aiScene* scene = importer.ReadFile(path.string(), flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            std::string error = importer.GetErrorString();
            CONSOLE_LOG(LEVEL_ERROR) << "Assimp import failed: " << (error.empty() ? "Unknown error" : error);
            return { nullptr, [](const aiScene*) {} };
        }

        // Return a unique_ptr that uses a no-op deleter, as Assimp's importer owns the memory.
        return { scene, [](const aiScene*) { /* Importer handles cleanup */ } };
    }

    void ExtractMeshTransformsFromNode(const aiNode* node, const Mat4& parentTransform, const aiScene* scene, std::vector<std::pair<uint32_t, Mat4>>* meshTransforms)
    {
        if (!node)
            return;

        // Convert Assimp matrix to GLM matrix
        const aiMatrix4x4& aiMat = node->mTransformation;
        const Mat4 nodeTransform{ aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1, aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2, aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3, aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4 };
        const Mat4 globalTransform = parentTransform * nodeTransform;

        // Extract mesh and material for each mesh in this node
        for (uint32_t i = 0; i < node->mNumMeshes; ++i)
        {
            const uint32_t meshIndex = node->mMeshes[i];
            if (meshIndex >= scene->mNumMeshes)
                return;

            meshTransforms->emplace_back(meshIndex, globalTransform);
        }

        // Recursively process child nodes
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
            ExtractMeshTransformsFromNode(node->mChildren[i], globalTransform, scene, meshTransforms);
    }


    bool ImportFileFBX(const std::filesystem::path& filepath, std::vector<MeshHandle>* outMeshHandles, std::vector<std::pair<uint32_t, Mat4>>* outMeshTransforms, std::vector<MaterialHandle>* outMaterialHandles)
    {
        try
        {
            // Step 1: Import scene file from disk
            auto assimpScenePtr = internal::ImportScene(filepath);
            if (!assimpScenePtr)
            {
                CONSOLE_LOG(LEVEL_ERROR) << "Failed to import scene file: " << filepath.string();
                return false;
            }
            AssetLoading::LoadingConfig config{ AssetLoading::LoadingConfig::createBalanced() };
            const aiScene* scene = assimpScenePtr.get();
            const std::string basePath = filepath.string();
            const std::string baseDir = filepath.parent_path().string();

            // Step 2: Extract scene hierarchy (lights, cameras, objects)
            // For us, no need to do this cause we're only extracting the meshes and materials

            // Step 3: Process materials from Assimp data
            auto materialPtrs{ AssetLoading::MaterialLoading::collectMaterialPointers(scene, config) };
            std::vector<AssetLoading::ProcessedMaterial> processedMaterials;
            processedMaterials.reserve(materialPtrs.size());
            for (uint32_t i = 0; i < materialPtrs.size(); ++i)
                processedMaterials.push_back(AssetLoading::MaterialLoading::extractMaterial(materialPtrs[i], i, basePath, baseDir, scene));

            // Step 4: Process textures (dependent on materials)
            auto textureSources{ AssetLoading::TextureLoading::collectUniqueTextures(processedMaterials) };
            std::vector<AssetLoading::ProcessedTexture> processedTextures;
            processedTextures.reserve(textureSources.size());
            for (const auto& source : textureSources)
                processedTextures.push_back(AssetLoading::TextureLoading::extractTexture(source, config));

            // Step 5: Process meshes from Assimp data
            auto meshPtrs{ AssetLoading::MeshLoading::collectMeshPointers(scene, config) };
            std::vector<AssetLoading::ProcessedMesh> processedMeshes;
            processedMeshes.reserve(meshPtrs.size());
            for (uint32_t i = 0; i < meshPtrs.size(); ++i)
                processedMeshes.push_back(AssetLoading::MeshLoading::extractMesh(meshPtrs[i], i, config));

            // Step 6: Upload assets to the GPU
            auto graphicsAssetSystem{ ST<GraphicsAssets>::Get()->INTERNAL_GetAssetSystem() };
            std::vector<TextureHandle> textureHandles;
            textureHandles.reserve(processedTextures.size());
            for (const auto& texture : processedTextures)
                textureHandles.push_back(graphicsAssetSystem->createTexture(texture));

            outMaterialHandles->reserve(processedMaterials.size());
            for (const auto& material : processedMaterials)
                outMaterialHandles->push_back(graphicsAssetSystem->createMaterial(material));

            outMeshHandles->reserve(processedMeshes.size());
            for (const auto& mesh : processedMeshes)
                outMeshHandles->push_back(graphicsAssetSystem->createMesh(mesh));

            // Step 7: Finalize scene
            ExtractMeshTransformsFromNode(scene->mRootNode, mat4(1.0f), scene, outMeshTransforms);

            graphicsAssetSystem->FlushUploads();
        }
        catch (const std::exception& e)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Scene loading exception: " << e.what();
            return false;
        }

        return true;
    }

    void SetResourceHandlesFBX(const std::vector<AssociatedResourceHashes>& resourceHashes, const std::vector<MeshHandle>& meshHandles, const std::vector<std::pair<uint32_t, Mat4>> meshTransforms, const std::vector<MaterialHandle>& materialHandles)
    {
        auto& meshes{ ST<ResourceManager>::Get()->INTERNAL_GetMeshes() };
        auto& materials{ ST<ResourceManager>::Get()->INTERNAL_GetMaterials() };
        const auto& meshHashes{ resourceHashes[0].hashes };
        const auto& materialHashes{ resourceHashes[1].hashes };

        std::vector<ResourceMesh*> meshesCache;
        for (size_t i{}; i < meshHashes.size(); ++i)
        {
            ResourceMesh* mesh{ meshes.INTERNAL_GetResource(meshHashes[i], true) };
            mesh->handle = meshHandles[i];
            meshesCache.push_back(mesh);
        }

        for (const auto& [index, mat] : meshTransforms)
            meshesCache[index]->transform = mat;

        for (size_t i{}; i < materialHashes.size(); ++i)
            materials.INTERNAL_GetResource(materialHashes[i], true)->handle = materialHandles[i];
    }

}


bool ResourceFiletypeImporterFBX::Import(const std::filesystem::path& relativeFilepath)
{
    // Load the meshes and materials within the file
    std::vector<MeshHandle> meshHandles;
    std::vector<MaterialHandle> materialHandles;
    std::vector<std::pair<uint32_t, Mat4>> meshTransforms;
    if (!internal::ImportFileFBX(ST<Filepaths>::Get()->assets + "/" + relativeFilepath.string(), &meshHandles, &meshTransforms, &materialHandles))
        return false;

    // Check if the resources are already registered
    const auto* existingFileEntry{ ST<ResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(relativeFilepath) };
    // If it exists, ensure there are the correct number of resources loaded from this file
    if (existingFileEntry && !CheckNumMeshesAndMaterialsAreConsistent(existingFileEntry, meshHandles.size(), materialHandles.size()))
        // Num meshes and materials don't line up, maybe the fbx file changed. Replace the existing FileEntry and invalidate all resources currently associated with it.
        existingFileEntry = nullptr;
    // If it doesn't exist, create new resources
    if (!existingFileEntry)
        existingFileEntry = CreateNewFileEntry(relativeFilepath, meshHandles.size(), materialHandles.size());

    // Set the resources to the loaded indexes
    internal::SetResourceHandlesFBX(existingFileEntry->associatedResources, meshHandles, meshTransforms, materialHandles);

    return true;
}

bool ResourceFiletypeImporterFBX::CheckNumMeshesAndMaterialsAreConsistent(const ResourceFilepaths::FileEntry* fileEntry, size_t numMeshes, size_t numMaterials)
{
    // We assume [0] are meshes and [1] are materials
    if (fileEntry->associatedResources.size() != 2)
        return false;
    return fileEntry->associatedResources[0].hashes.size() == numMeshes &&
           fileEntry->associatedResources[1].hashes.size() == numMaterials;
}

const ResourceFilepaths::FileEntry* ResourceFiletypeImporterFBX::CreateNewFileEntry(const std::filesystem::path& relativeFilepath, size_t numMeshes, size_t numMaterials)
{
    std::vector<AssociatedResourceHashes> resourceHashes{ 2 };
    GenerateHashesForResourceType<ResourceMesh>(&resourceHashes[0], numMeshes);
    GenerateHashesForResourceType<ResourceMaterial>(&resourceHashes[1], numMaterials);

    GenerateNamesForResources(resourceHashes, relativeFilepath);
    return ST<ResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(relativeFilepath, std::move(resourceHashes));
}
