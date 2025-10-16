/******************************************************************************/
/*!
\file   ResourceFiletypeImporterFBX.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for FBX files.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ResourceFiletypeImporterFBX.h"
#include "ResourceTypesGraphics.h"
// The FBX importer actually calls our asset compiler which produces .mesh and .ktx2 files.
// We then delegate the importing of these .mesh and .ktx2 files to those respective importers.
// Therefore, we need to call the resource importer again to process those files.
#include "SceneCompiler.h"
#include "GameSettings.h"
#include "ResourceImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "import_config.h"
#include "material_loader.h"
#include "texture_loader.h"
#include "mesh_loader.h"
#include "GraphicsAPI.h"

namespace internal {

    struct RawResourcesFBX
    {
        std::vector<MeshHandle> meshHandles;
        std::vector<std::pair<uint32_t, Mat4>> meshTransforms;
        std::vector<MaterialHandle> materialHandles;
        std::vector<TextureHandle> textureHandles;
    };

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


    bool ImportFileFBX(const std::filesystem::path& filepath, RawResourcesFBX* outRawResources)
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
            outRawResources->textureHandles.reserve(processedTextures.size());
            for (const auto& texture : processedTextures)
                outRawResources->textureHandles.push_back(graphicsAssetSystem->createTexture(texture));

            outRawResources->materialHandles.reserve(processedMaterials.size());
            for (const auto& material : processedMaterials)
                outRawResources->materialHandles.push_back(graphicsAssetSystem->createMaterial(material));

            outRawResources->meshHandles.reserve(processedMeshes.size());
            for (const auto& mesh : processedMeshes)
                outRawResources->meshHandles.push_back(graphicsAssetSystem->createMesh(mesh));

            // Step 7: Finalize scene
            ExtractMeshTransformsFromNode(scene->mRootNode, mat4(1.0f), scene, &outRawResources->meshTransforms);

            graphicsAssetSystem->FlushUploads();
        }
        catch (const std::exception& e)
        {
            CONSOLE_LOG(LEVEL_ERROR) << "Scene loading exception: " << e.what();
            return false;
        }

        return true;
    }

    void SetResourceHandlesFBX(const std::vector<AssociatedResourceHashes>& resourceHashes, RawResourcesFBX&& rawResources)
    {
        auto& meshes{ ST<ResourceManager>::Get()->INTERNAL_GetMeshes() };
        auto& materials{ ST<ResourceManager>::Get()->INTERNAL_GetMaterials() };
        auto& textures{ ST<ResourceManager>::Get()->INTERNAL_GetTextures() };
        const auto& meshHashes{ resourceHashes[0].hashes };
        const auto& materialHashes{ resourceHashes[1].hashes };
        const auto& textureHashes{ resourceHashes[2].hashes };

        ResourceMesh* mesh{ meshes.INTERNAL_GetResource(meshHashes[0], true) };
        mesh->handles = std::move(rawResources.meshHandles);
        mesh->transforms.resize(rawResources.meshTransforms.size());
        for (auto&& [index, mat] : rawResources.meshTransforms)
            mesh->transforms[index] = std::move(mat);

        for (size_t i{}; i < materialHashes.size(); ++i)
            materials.INTERNAL_GetResource(materialHashes[i], true)->handle = std::move(rawResources.materialHandles[i]);

        for (size_t i{}; i < textureHashes.size(); ++i)
            textures.INTERNAL_GetResource(textureHashes[i], true)->handle = std::move(rawResources.textureHandles[i]);
    }

}


bool ResourceFiletypeImporterFBX::Import(const std::filesystem::path& assetRelativeFilepath)
{
    // Set up compile options
    compiler::SceneCompiler compiler;
    compiler::CompilerOptions options;
    options.general.inputPath = GetExeRelativeFilepath(assetRelativeFilepath);
    options.general.outputPath = ST<Filepaths>::Get()->assets + "/CompiledAssets/";

    // Ask the compiler to compile the .fbx into .mesh and .ktx2
    compiler::CompilationResult result = compiler.Compile(options);
    if (!result.success)
        return false;

    // Delegate importing of the created files to their respective importers
    for (const auto& path : result.createdMeshFiles)
        ResourceImporter::Import(path);
    for (const auto& path : result.createdTextureFiles)
        ResourceImporter::Import(path);
    for (const auto& path : result.createdMaterialFiles)
        ResourceImporter::Import(path);

    return true;
}
