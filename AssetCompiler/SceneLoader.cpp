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
    // ----- Helpers ----- //
    vec3 AiToVec3(const aiVector3D& v)
    {
        return vec3(v.x, v.y, v.z);
    }

    quat AiToQuat(const aiQuaternion& q)
    {
        return quat(q.w, q.x, q.y, q.z); // Note: GLM/quat constructor is (w, x, y, z)
    }

    mat4 AiToMat4(const aiMatrix4x4& aiMat)
    {
        return mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
    }

    void buildNodeTransformMap(const aiNode* node, const mat4& parentTransform, std::map<std::string, mat4>& transformMap)
    {
        if (!node) return;

        std::string nodeName(node->mName.C_Str());
        mat4 globalTransform = parentTransform * AiToMat4(node->mTransformation);

        if (!nodeName.empty())
        {
            transformMap[nodeName] = globalTransform;
        }

        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            buildNodeTransformMap(node->mChildren[i], globalTransform, transformMap);
        }
    }

    void accumulateWeight(SkinningData& data, uint32_t jointIndex, float weight)
    {
        if (weight <= 0.0f) return;

        // Find an empty slot
        for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
        {
            if (data.weights[i] == 0.0f)
            {
                data.boneIndices[i] = jointIndex;
                data.weights[i] = weight;
                return;
            }
        }

        // If no empty slot, replace the one with the smallest weight
        size_t smallest = 0;
        for (size_t i = 1; i < MAX_BONES_PER_VERTEX; ++i)
        {
            if (data.weights[i] < data.weights[smallest])
            {
                smallest = i;
            }
        }

        if (weight > data.weights[smallest])
        {
            data.boneIndices[smallest] = jointIndex;
            data.weights[smallest] = weight;
        }
    }

    void normalizeWeights(SkinningData& data)
    {
        float totalWeight = 0.0f;
        for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
        {
            totalWeight += data.weights[i];
        }

        if (totalWeight > 0.0f)
        {
            float inv = 1.0f / totalWeight;
            for (size_t i = 0; i < MAX_BONES_PER_VERTEX; ++i)
            {
                data.weights[i] *= inv;
            }
        }
        else
        {
            data = SkinningData{};
        }

    }

    // ----- Aniamtins ----- //
    void SceneLoader::extractSkeleton(const aiScene* scene, Scene& outScene)
    {
        std::map<std::string, mat4> globalNodeTransforms;
        buildNodeTransformMap(scene->mRootNode, mat4(1.0f), globalNodeTransforms);

        // Build Bone Map & Get mOffsetMatrix (mesh space -> bone space in bind pose)
        for (uint32_t meshIdx = 0; meshIdx < scene->mNumMeshes; ++meshIdx)
        {
            const aiMesh* aiMesh = scene->mMeshes[meshIdx];
            if (!aiMesh->HasBones()) continue;

            for (uint32_t boneIdx = 0; boneIdx < aiMesh->mNumBones; ++boneIdx)
            {
                const aiBone* aiBone = aiMesh->mBones[boneIdx];
                std::string boneName(aiBone->mName.C_Str());

                // If bone is not in our map yet, add it
                if (outScene.skeleton.boneNameToIndex.find(boneName) == outScene.skeleton.boneNameToIndex.end())
                {
                    uint32_t index = outScene.skeleton.bones.size();
                    outScene.skeleton.boneNameToIndex[boneName] = index;

                    ProcessedBone bone;
                    bone.name = boneName;
                    bone.inverseBindPose = AiToMat4(aiBone->mOffsetMatrix);

                    if (globalNodeTransforms.count(boneName))
                    {
                        bone.bindPose = globalNodeTransforms[boneName];
                    }
                    else
                    {
                        bone.bindPose = mat4(1.0f);
                    }

                    outScene.skeleton.bones.push_back(bone);
                }
            }
        }

        buildBoneHierarchy(scene->mRootNode, -1, outScene.skeleton);
    }

    void SceneLoader::buildBoneHierarchy(const aiNode* node, int32_t parentBoneIndex, ProcessedSkeleton& skeleton)
    {
        std::string nodeName(node->mName.C_Str());
        int32_t currentBoneIndex = -1; // Default to -1 (not a bone)

        // If a node represents a bone in the hierarchy, then the node name must match the bone name.
        // Check if this node is a bone we've already registered
        auto it = skeleton.boneNameToIndex.find(nodeName);
        if (it != skeleton.boneNameToIndex.end())
        {
            // It's a bone! Get its index.
            currentBoneIndex = it->second;
            // Set its parent
            skeleton.bones[currentBoneIndex].parentIndex = parentBoneIndex;
        }

        // Recurse for all children
        for (uint32_t i = 0; i < node->mNumChildren; ++i)
        {
            // If this node was a bone, it's the parent for its children.
            // If it wasn't, we just pass down the parentBoneIndex we were given.
            buildBoneHierarchy(node->mChildren[i], (currentBoneIndex != -1) ? currentBoneIndex : parentBoneIndex, skeleton);
        }
    }

    void SceneLoader::extractAnimations(const aiScene* scene, Scene& outScene)
    {
        outScene.animations.reserve(scene->mNumAnimations);

        for (uint32_t i = 0; i < scene->mNumAnimations; ++i)
        {
            const aiAnimation* aiAnim = scene->mAnimations[i];
            ProcessedAnimation anim;
            anim.name = aiAnim->mName.C_Str();

            // If name is empty, generate one
            if (anim.name.empty())
            {
                anim.name = "Anim_" + std::to_string(i);
            }

            // Assimp stores duration in ticks. Convert to seconds.
            anim.ticksPerSecond = aiAnim->mTicksPerSecond > 0.0 ? static_cast<float>(aiAnim->mTicksPerSecond) : 1.0f;
            anim.duration = aiAnim->mDuration > 0.0 ? static_cast<float>(aiAnim->mDuration / anim.ticksPerSecond) : 0.0f;

            anim.skeletalChannels.reserve(aiAnim->mNumChannels);

            for (uint32_t j = 0; j < aiAnim->mNumChannels; ++j)
            {
                const aiNodeAnim* aiChannel = aiAnim->mChannels[j];
                std::string nodeName(aiChannel->mNodeName.C_Str());

                // Find the bone index this channel animates
                auto it = outScene.skeleton.boneNameToIndex.find(nodeName);
                if (it == outScene.skeleton.boneNameToIndex.end())
                {
                    // This channel animates a node that is not part of our skeleton
                    continue;
                }

                ProcessedBoneChannel pbc;
                pbc.boneIndex = it->second;

                // Copy keyframes
                pbc.positionKeys.resize(aiChannel->mNumPositionKeys);
                for (uint32_t k = 0; k < aiChannel->mNumPositionKeys; ++k)
                {
                    pbc.positionKeys[k].time = (float)aiChannel->mPositionKeys[k].mTime / anim.ticksPerSecond;
                    pbc.positionKeys[k].value = AiToVec3(aiChannel->mPositionKeys[k].mValue);
                }

                pbc.rotationKeys.resize(aiChannel->mNumRotationKeys);
                for (uint32_t k = 0; k < aiChannel->mNumRotationKeys; ++k)
                {
                    pbc.rotationKeys[k].time = (float)aiChannel->mRotationKeys[k].mTime / anim.ticksPerSecond;
                    pbc.rotationKeys[k].value = AiToQuat(aiChannel->mRotationKeys[k].mValue);
                }

                pbc.scaleKeys.resize(aiChannel->mNumScalingKeys);
                for (uint32_t k = 0; k < aiChannel->mNumScalingKeys; ++k)
                {
                    pbc.scaleKeys[k].time = (float)aiChannel->mScalingKeys[k].mTime / anim.ticksPerSecond;
                    pbc.scaleKeys[k].value = AiToVec3(aiChannel->mScalingKeys[k].mValue);
                }

                anim.skeletalChannels.push_back(pbc);
            }
            outScene.animations.push_back(anim);
        }
    }



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
            //std::cout << "Material says it wants: " << texturePath << "\n";
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
            vertex.position = AiToVec3(aiMesh->mVertices[i]);

            // Normal
            if (aiMesh->mNormals)
            {
                vertex.normal = vec3(aiMesh->mNormals[i].x, aiMesh->mNormals[i].y, aiMesh->mNormals[i].z);
            }
            else
            {
                vertex.normal = AiToVec3(aiMesh->mNormals[i]);
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

    ProcessedMesh SceneLoader::extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const MeshOptions& options, const ProcessedSkeleton& skeleton)
    {
        ProcessedMesh mesh;

        // Extract name and material index
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

        if (aiMesh->HasBones() && mesh.vertices.size() > 0)
        {
            mesh.skinning.resize(mesh.vertices.size());

            for (uint32_t boneIdx = 0; boneIdx < aiMesh->mNumBones; ++boneIdx)
            {
                const aiBone* aiBone = aiMesh->mBones[boneIdx];
                std::string boneName(aiBone->mName.C_Str());

                auto it = skeleton.boneNameToIndex.find(boneName);
                if (it == skeleton.boneNameToIndex.end()) continue;

                uint32_t globalBoneIndex = it->second;

                // Add this bone's weights to all affected vertices
                for (uint32_t weightIdx = 0; weightIdx < aiBone->mNumWeights; ++weightIdx)
                {
                    const aiVertexWeight& weight = aiBone->mWeights[weightIdx];

                    // Get the SkinningData for this vertex
                    SkinningData& vertexSkinData = mesh.skinning[weight.mVertexId];

                    // Add the weight
                    accumulateWeight(vertexSkinData, globalBoneIndex, weight.mWeight);
                }
            }

            for (SkinningData& skinData : mesh.skinning)
            {
                normalizeWeights(skinData);
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
                loadedScene.materials.push_back(extractMaterialSlot(materialPtrs[i], i, baseDir));
            }

            // Build the bone map (bone info, bone parents, construct bone hierarchy)
            extractSkeleton(scene, loadedScene);
            // Get animation information - timing + keyframe information for each bone
            extractAnimations(scene, loadedScene);

            // Process meshes
            auto meshPtrs = collectMeshPointers(scene, meshOptions);
            loadedScene.meshes.reserve(meshPtrs.size());
            for (uint32_t i = 0; i < meshPtrs.size(); ++i)
            {
                loadedScene.meshes.push_back(extractMesh(meshPtrs[i], i, meshOptions, loadedScene.skeleton));
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