/******************************************************************************/
/*!
\file   SceneLoader.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Loads an fbx scene, and extracts the meshes, node hierarchy, materials, and referenced textures

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "SceneLoader.h"

#include <assimp/material.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>

#include <numeric>
#include <iostream>

namespace compiler
{
    // ----- Material ----- //
    std::vector<const aiMaterial*> SceneLoader::collectMaterialPointers(const aiScene* scene)
    {
        if (!scene || scene->mNumMaterials == 0)
        {
            return {};
        }

        std::vector<const aiMaterial*> materialPtrs;
        materialPtrs.reserve(scene->mNumMaterials);

        for (uint32_t i = 0; i < scene->mNumMaterials; ++i)
        {
            if (scene->mMaterials[i])
            {
                materialPtrs.push_back(scene->mMaterials[i]);
            }
        }

        return materialPtrs;
    }

    bool SceneLoader::extractTexturePath(const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath)
    {
        aiString path;
        if (aiMat->GetTexture(type, 0, &path) == AI_SUCCESS)
        {
            std::filesystem::path texturePath(path.C_Str());
            if (texturePath.is_relative())
            {
                outSlot.texturePaths[key] = modelBasePath / texturePath;
            }
            else
            {
                outSlot.texturePaths[key] = texturePath;
            }
            return true;
        }
        return false;
    }

    ProcessedMaterialSlot SceneLoader::extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath)
    {
        ProcessedMaterialSlot slot;
        slot.originalIndex = materialIndex;

        aiString name;
        if (aiMat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS && strlen(name.C_Str()) > 0)
        {
            slot.name = name.C_Str();
        }
        else
        {
            slot.name = "Material_" + std::to_string(materialIndex);
        }

        if (!extractTexturePath(aiMat, aiTextureType_BASE_COLOR, "baseColor", slot, modelBasePath))
            extractTexturePath(aiMat, aiTextureType_DIFFUSE, "baseColor", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_METALNESS, "metallicRoughness", slot, modelBasePath);
        if (!extractTexturePath(aiMat, aiTextureType_NORMALS, "normal", slot, modelBasePath))
            extractTexturePath(aiMat, aiTextureType_HEIGHT, "normal", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_EMISSIVE, "emissive", slot, modelBasePath);
        extractTexturePath(aiMat, aiTextureType_AMBIENT_OCCLUSION, "occlusion", slot, modelBasePath);

        // Get material parameter values
        aiColor4D color;
        float factor;

        // Base color
        if (aiMat->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS)
        {
            slot.baseColorFactor = { color.r, color.g, color.b, color.a };
        }
        else if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS)
        {
            slot.baseColorFactor = vec4(color.r, color.g, color.b, 1.0f);
        }
        else if (aiMat->Get("$clr.diffuse", 0, 0, color) == AI_SUCCESS)
        {
            slot.baseColorFactor = vec4(color.r, color.g, color.b, 1.0f);
        }

        // Metallic
        if (aiMat->Get(AI_MATKEY_METALLIC_FACTOR, factor) == AI_SUCCESS)
        {
            slot.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
        }
        else if (aiMat->Get("$mat.metallicFactor", 0, 0, factor) == AI_SUCCESS)
        {
            slot.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
        }
        else if (aiMat->Get(AI_MATKEY_REFLECTIVITY, factor) == AI_SUCCESS)
        {
            slot.metallicFactor = std::clamp(factor, 0.0f, 1.0f);
        }

        // Roughness
        if (aiMat->Get(AI_MATKEY_ROUGHNESS_FACTOR, factor) == AI_SUCCESS)
        {
            slot.roughnessFactor = std::clamp(factor, 0.0f, 1.0f);
        }
        else if (aiMat->Get("$mat.roughnessFactor", 0, 0, factor) == AI_SUCCESS)
        {
            slot.roughnessFactor = std::clamp(factor, 0.0f, 1.0f);
        }
        else if (aiMat->Get(AI_MATKEY_SHININESS, factor) == AI_SUCCESS)
        {
            slot.roughnessFactor = std::clamp(1.0f - (factor / 256.0f), 0.0f, 1.0f);
        }
        else if (aiMat->Get(AI_MATKEY_SHININESS_STRENGTH, factor) == AI_SUCCESS)
        {
            slot.roughnessFactor = std::clamp(1.0f - factor, 0.0f, 1.0f);
        }

        // Emissive
        if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS)
        {
            slot.emissiveFactor = vec3(color.r, color.g, color.b);
        }
        else if (aiMat->Get("$clr.emissive", 0, 0, color) == AI_SUCCESS)
        {
            slot.emissiveFactor = vec3(color.r, color.g, color.b);
        }

        // Normal scale
        if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_SCALE(aiTextureType_NORMALS, 0), factor) == AI_SUCCESS)
        {
            slot.normalScale = std::max(0.0f, factor);
        }

        // Occlusion strength
        if (aiMat->Get(AI_MATKEY_GLTF_TEXTURE_STRENGTH(aiTextureType_AMBIENT_OCCLUSION, 0), factor) == AI_SUCCESS)
        {
            slot.occlusionStrength = std::clamp(factor, 0.0f, 1.0f);
        }

        // Alpha properties
        if (aiMat->Get(AI_MATKEY_OPACITY, factor) == AI_SUCCESS)
        {
            slot.baseColorFactor.a = std::clamp(factor, 0.0f, 1.0f);
        }

        if (aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, factor) == AI_SUCCESS)
        {
            slot.alphaCutoff = std::clamp(factor, 0.0f, 1.0f);
        }

        aiString alphaModeStr;
        if (aiMat->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaModeStr) == AI_SUCCESS)
        {
            std::string mode = alphaModeStr.C_Str();
            if (mode == "OPAQUE")
            {
                slot.alphaMode = AlphaMode::Opaque;
            }
            else if (mode == "MASK")
            {
                slot.alphaMode = AlphaMode::Mask;
            }
            else if (mode == "BLEND")
            {
                slot.alphaMode = AlphaMode::Blend;
            }
        }
        else
        {
            // Fallback: detect transparency from alpha value
            if (slot.baseColorFactor.a < 0.99f)
            {
                slot.alphaMode = AlphaMode::Blend;
            }
            else
            {
                slot.alphaMode = AlphaMode::Opaque;
            }
        }

        // Alpha cutoff
        if (aiMat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, factor) == AI_SUCCESS)
        {
            slot.alphaCutoff = std::clamp(factor, 0.0f, 1.0f);
        }

        // Unlit detection
        int shadingModel;
        if (aiMat->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == AI_SUCCESS)
        {
            if (shadingModel == aiShadingMode_Unlit || shadingModel == aiShadingMode_NoShading)
            {
                slot.flags |= MaterialFlags::UNLIT;
            }
        }

        // Alternative unlit detection for various formats
        aiString unlitStr;
        if (aiMat->Get("unlit", 0, 0, unlitStr) == AI_SUCCESS)
        {
            std::string unlit = unlitStr.C_Str();
            if (unlit == "true" || unlit == "1")
            {
                slot.flags |= MaterialFlags::UNLIT;
            }
        }

        // Two-sided detection
        int twosided;
        if (aiMat->Get(AI_MATKEY_TWOSIDED, twosided) == AI_SUCCESS && twosided)
        {
            slot.flags |= MaterialFlags::DOUBLE_SIDED;
        }

        //The importer will materialType and flags for us
        //slot.materialTypeFlags = MaterialType::METALLIC_ROUGHNESS;
        //uint32_t alphaModeFlags = static_cast<uint32_t>(slot.alphaMode) & MaterialFlags::ALPHA_MODE_MASK;
        //slot.flags = (slot.flags & ~MaterialFlags::ALPHA_MODE_MASK) | alphaModeFlags;

        return slot;
    }


    // ----- Mesh ------ //
    std::vector<const aiMesh*> SceneLoader::collectMeshPointers(const aiScene* scene, [[maybe_unused]] const MeshOptions& options)
    {
        if (!scene || !scene->mMeshes || scene->mNumMeshes == 0) return {};

        std::vector<const aiMesh*> meshPtrs;
        meshPtrs.reserve(scene->mNumMeshes);

        for (uint32_t i = 0; i < scene->mNumMeshes; ++i)
        {
            if (scene->mMeshes[i] && scene->mMeshes[i]->mNumVertices > 0)
            {
                meshPtrs.push_back(scene->mMeshes[i]);
            }
        }
        return meshPtrs;
    }

    void SceneLoader::extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const MeshOptions& options)
    {
        vertices.resize(aiMesh->mNumVertices);
        for (uint32_t i = 0; i < aiMesh->mNumVertices; ++i)
        {
            Vertex& vertex = vertices[i];

            // Position (always present)
            vertex.position = vec3(aiMesh->mVertices[i].x, aiMesh->mVertices[i].y, aiMesh->mVertices[i].z);

            // Normal
            if (aiMesh->mNormals)
            {
                vertex.normal = vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
            }
            else
            {
                vertex.normal = vec3(0.0f, 1.0f, 0.0f);
            }

            if (aiMesh->mTextureCoords[0])
            {
                const float u = aiMesh->mTextureCoords[0][i].x;
                const float v = aiMesh->mTextureCoords[0][i].y;
                vertex.setUV(u, options.flipUVs ? (1.0f - v) : v);
            }
            else
            {
                vertex.setUV(0.0f, 0.0f);
            }

            // Initialize tangent
            vertex.tangent = vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }

    ProcessedMesh SceneLoader::extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const MeshOptions& options)
    {
        ProcessedMesh mesh;

        // 1. Extract name and material index
        mesh.name = aiMesh->mName.length > 0 ? aiMesh->mName.C_Str() : ("Mesh_" + std::to_string(meshIndex));
        mesh.materialIndex = aiMesh->mMaterialIndex;

        if (!aiMesh->mVertices || aiMesh->mNumVertices == 0)
        {
            return mesh; // Return empty mesh
        }

        extractVertices(aiMesh, mesh.vertices, options);

        mesh.indices.reserve(aiMesh->mNumFaces * 3);
        for (uint32_t i = 0; i < aiMesh->mNumFaces; ++i)
        {
            const aiFace& face = aiMesh->mFaces[i];
            if (face.mNumIndices == 3)
            {
                mesh.indices.push_back(face.mIndices[0]);
                mesh.indices.push_back(face.mIndices[1]);
                mesh.indices.push_back(face.mIndices[2]);
            }
        }
        return mesh;
    }

    // ----- Scene ----- //
    SceneLoadData SceneLoader::loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions)
    {
        SceneLoadData returnData;

        try
        {
            // Import scene file from disk using Assimp.
            auto assimpScenePtr = importScene(path);
            if (!assimpScenePtr)
            {
                returnData.errors.push_back("Failed to import scene file with Assimp: " + path.string());
                return returnData;
            }

            const aiScene* scene = assimpScenePtr.get();
            const auto baseDir = path.parent_path();

            // We incrementally build up the scene object
            // Cotnains: nodes, meshes, materials
            Scene loadedScene;
            loadedScene.name = path.filename().string();

            // Extract scene hierarchy (nodes).
            if (scene->mRootNode)
            {
                extractNodesForCompiler(scene->mRootNode, -1, scene, loadedScene.nodes);
            }

            // Process materials
            auto materialPtrs = collectMaterialPointers(scene);
            loadedScene.materials.reserve(materialPtrs.size());
            for (uint32_t i = 0; i < materialPtrs.size(); ++i)
            {
                // passing the base directory to correctly resolve relative texture paths
                loadedScene.materials.push_back(
                    extractMaterialSlot(materialPtrs[i], i, baseDir)
                );
            }

            // Process meshes
            auto meshPtrs = collectMeshPointers(scene, meshOptions);
            loadedScene.meshes.reserve(meshPtrs.size());
            for (uint32_t i = 0; i < meshPtrs.size(); ++i)
            {
                loadedScene.meshes.push_back(extractMesh(meshPtrs[i], i, meshOptions));
            }

            returnData.scene = std::move(loadedScene);
        }
        catch (const std::exception& e)
        {
            returnData.errors.push_back("An exception occurred during scene loading: " + std::string(e.what()));
        }

        return returnData;
    }

    void SceneLoader::reportProgress(const ProgressCallback& callback, float progress, const std::string& status)
    {
        if (callback)
        {
            callback(progress, status);
        }
    }

    std::unique_ptr<const aiScene, void(*)(const aiScene*)> SceneLoader::importScene(const std::filesystem::path& path) const
    {
        if (!exists(path))
        {
            //LOG_ERROR("Scene file not found: {}", path.string());
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
            //LOG_ERROR("Assimp import failed: {}", error.empty() ? "Unknown error" : error);
            return { nullptr, [](const aiScene*) {} };
        }

        // Return a unique_ptr that uses a no-op deleter, as Assimp's importer owns the memory.
        return { scene,
            [](const aiScene*)
            {
                /* Importer handles cleanup */
                } };
    }



    void SceneLoader::extractNodesForCompiler(
        const aiNode* node,
        const int32_t parentIdx,
        const aiScene* scene,
        std::vector<SceneNode>& outNodes) const
    {
        if (!node)
        {
            return;
        }

        SceneNode primaryNode;
        primaryNode.parentIndex = parentIdx;

        // Convert Assimp matrix to GLM matrix
        const aiMatrix4x4& aiMat = node->mTransformation;

        primaryNode.localTransform = mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
        primaryNode.name = node->mName.length > 0 ? node->mName.C_Str() : "";

        int32_t currentNodeIndex = -1;

        // 2. Handle the meshes for this node.
        if (node->mNumMeshes > 0)
        {
            // The first mesh is assigned to our primary node.
            primaryNode.meshIndex = node->mMeshes[0];
            outNodes.push_back(primaryNode);
            currentNodeIndex = static_cast<int32_t>(outNodes.size()) - 1;

            // If there are more meshes, create sibling nodes for them.
            for (uint32_t i = 1; i < node->mNumMeshes; ++i)
            {
                SceneNode siblingNode = primaryNode; // Copy parent, transform, and name
                siblingNode.meshIndex = node->mMeshes[i];
                outNodes.push_back(siblingNode);
            }
        }
        else
        {
            // If there are no meshes, we still add the node to preserve the hierarchy.
            // Its meshIndex will remain -1.
            outNodes.push_back(primaryNode);
            currentNodeIndex = static_cast<int32_t>(outNodes.size()) - 1;
        }

        // Recursively process child nodes
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            extractNodesForCompiler(node->mChildren[i], currentNodeIndex, scene, outNodes);
        }
    }


}