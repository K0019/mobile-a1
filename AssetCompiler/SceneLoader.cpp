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

All content � 2025 DigiPen Institute of Technology Singapore.
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
    // ----- Winding Order Detection and Fix ----- //
    // Analyzes mesh winding order by comparing geometric face normals with vertex normals
    // CCW convention: cross(edge1, edge2) where edge1 = v1-v0, edge2 = v2-v0
    // Returns: 1 = CCW (correct), -1 = CW (inverted), 0 = mixed/unknown
    int DetectWindingOrder(const ProcessedMesh& mesh)
    {
        if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3 != 0)
            return 0;

        int ccwAgree = 0;   // Face normal (CCW) agrees with vertex normal
        int ccwDisagree = 0;

        const uint32_t numTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);

        for (uint32_t tri = 0; tri < numTriangles; ++tri)
        {
            uint32_t i0 = mesh.indices[tri * 3 + 0];
            uint32_t i1 = mesh.indices[tri * 3 + 1];
            uint32_t i2 = mesh.indices[tri * 3 + 2];

            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
                continue;

            const vec3& p0 = mesh.vertices[i0].position;
            const vec3& p1 = mesh.vertices[i1].position;
            const vec3& p2 = mesh.vertices[i2].position;

            // Compute face normal using CCW convention: cross(v1-v0, v2-v0)
            vec3 edge1 = p1 - p0;
            vec3 edge2 = p2 - p0;
            vec3 faceNormalCCW = glm::cross(edge1, edge2);

            float len = glm::length(faceNormalCCW);
            if (len < 0.0001f)
                continue; // Skip degenerate triangles

            faceNormalCCW /= len;

            // Average vertex normals
            vec3 avgVertexNormal = glm::normalize(
                mesh.vertices[i0].normal + mesh.vertices[i1].normal + mesh.vertices[i2].normal);

            float dot = glm::dot(faceNormalCCW, avgVertexNormal);
            if (dot > 0.0f)
                ccwAgree++;
            else
                ccwDisagree++;
        }

        // Determine winding
        if (ccwAgree > ccwDisagree * 2)
            return 1;  // CCW (correct for Vulkan)
        else if (ccwDisagree > ccwAgree * 2)
            return -1; // CW (inverted)
        else
            return 0;  // Mixed/unknown
    }

    // Fixes CW winding by reversing triangle indices to CCW and flipping normals
    void FixWindingOrder(ProcessedMesh& mesh)
    {
        int winding = DetectWindingOrder(mesh);

        if (winding == -1)
        {
            // CW winding detected - reverse each triangle's indices to make it CCW
            const uint32_t numTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);
            for (uint32_t tri = 0; tri < numTriangles; ++tri)
            {
                // Swap indices 1 and 2 to reverse winding: (0,1,2) -> (0,2,1)
                std::swap(mesh.indices[tri * 3 + 1], mesh.indices[tri * 3 + 2]);
            }

            // Also flip all vertex normals since we reversed the geometric facing
            // Without this, normals would point "into" the surface after winding fix
            for (auto& v : mesh.vertices)
            {
                v.normal = -v.normal;
            }

            std::cerr << "[WINDING FIX] Mesh '" << mesh.name << "': Reversed " << numTriangles
                      << " triangles from CW to CCW and flipped " << mesh.vertices.size() << " normals\n" << std::flush;
        }
    }

    void DiagnoseWindingOrder(const ProcessedMesh& mesh, const std::string& context)
    {
        if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3 != 0)
            return;

        int winding = DetectWindingOrder(mesh);
        const char* windingStr = "UNKNOWN";
        if (winding == 1)
            windingStr = "CCW (correct for Vulkan)";
        else if (winding == -1)
            windingStr = "CW (INVERTED for Vulkan)";
        else
            windingStr = "MIXED (inconsistent)";

        const uint32_t numTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);
        std::cerr << "[WINDING DIAGNOSTIC] " << context << " - Mesh '" << mesh.name << "':\n"
                  << "  Triangles: " << numTriangles << "\n"
                  << "  Detected winding: " << windingStr << "\n" << std::flush;
    }

    // Fixes vertex normals for meshes with inconsistent or inverted normals by regenerating from geometry
    // Uses area-weighted smooth normals (unnormalized cross product = 2x area)
    // Called AFTER FixWindingOrder, so winding should be CCW. Handles:
    // - winding == 0 (MIXED): some normals point wrong way
    // - winding == -1 (INVERTED): all normals point wrong way (can happen after winding fix)
    void FixInconsistentNormals(ProcessedMesh& mesh)
    {
        int winding = DetectWindingOrder(mesh);
        if (winding == 1) // Only skip if normals are already correct (CCW agreement)
            return;

        if (mesh.vertices.empty() || mesh.indices.empty() || mesh.indices.size() % 3 != 0)
            return;

        const char* issueType = (winding == 0) ? "MIXED" : "INVERTED";
        std::cerr << "[NORMAL FIX] Mesh '" << mesh.name << "': Detected " << issueType << " normals, regenerating from geometry ("
                  << mesh.vertices.size() << " vertices, " << mesh.indices.size() / 3 << " triangles)\n" << std::flush;

        // Initialize all normals to zero
        for (auto& v : mesh.vertices)
            v.normal = vec3(0.0f);

        const uint32_t numTriangles = static_cast<uint32_t>(mesh.indices.size() / 3);
        int validTris = 0;

        // Accumulate area-weighted face normals to each vertex
        for (uint32_t tri = 0; tri < numTriangles; ++tri)
        {
            uint32_t i0 = mesh.indices[tri * 3 + 0];
            uint32_t i1 = mesh.indices[tri * 3 + 1];
            uint32_t i2 = mesh.indices[tri * 3 + 2];

            if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size())
                continue;

            const vec3& p0 = mesh.vertices[i0].position;
            const vec3& p1 = mesh.vertices[i1].position;
            const vec3& p2 = mesh.vertices[i2].position;

            vec3 e1 = p1 - p0;
            vec3 e2 = p2 - p0;
            vec3 faceNormal = glm::cross(e1, e2); // Unnormalized = area-weighted

            float area2 = glm::length(faceNormal);
            if (area2 < 0.0000001f)
                continue; // Skip degenerate triangles

            validTris++;
            // Add unnormalized (area-weighted) normal to each vertex
            mesh.vertices[i0].normal += faceNormal;
            mesh.vertices[i1].normal += faceNormal;
            mesh.vertices[i2].normal += faceNormal;
        }

        // Normalize all vertex normals
        int fixed = 0;
        for (auto& v : mesh.vertices)
        {
            float len = glm::length(v.normal);
            if (len > 0.0001f)
            {
                v.normal /= len;
                fixed++;
            }
            else
            {
                v.normal = vec3(0.0f, 1.0f, 0.0f); // Fallback up normal
            }
        }

        std::cerr << "[NORMAL FIX] Mesh '" << mesh.name << "': Regenerated " << fixed
                  << " vertex normals from " << validTris << " valid triangles\n" << std::flush;
    }

    // ----- Helpers ----- //
    inline vec3 AiToVec3(const aiVector3D& v)
    {
        return vec3(v.x, v.y, v.z);
    }

    inline quat AiToQuat(const aiQuaternion& q)
    {
        return quat(q.w, q.x, q.y, q.z); // Note: GLM/quat constructor is (w, x, y, z)
    }

    inline mat4 AiToMat4(const aiMatrix4x4& aiMat)
    {
        return mat4(aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4);
    }

    inline float ticksToSeconds(double time, double ticksPerSecond)
    {
        if (ticksPerSecond <= 0.0)
            return static_cast<float>(time);
        return static_cast<float>(time / ticksPerSecond);
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
                    uint32_t index = static_cast<uint32_t>(outScene.skeleton.bones.size());
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

            //Skeleteal animations
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
                pbc.nodeName = nodeName;

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
            
            //Morph Animations
            anim.morphChannels.reserve(aiAnim->mNumMorphMeshChannels);
            for (uint32_t j = 0; j < aiAnim->mNumMorphMeshChannels; ++j)
            {
                const aiMeshMorphAnim* aiMorphChannel = aiAnim->mMorphMeshChannels[j];

                ProcessedMorphChannel morphChannel;
                morphChannel.meshName = aiMorphChannel->mName.C_Str();
                morphChannel.keys.reserve(aiMorphChannel->mNumKeys);

                for (uint32_t k = 0; k < aiMorphChannel->mNumKeys; ++k)
                {
                    const aiMeshMorphKey& aiKey = aiMorphChannel->mKeys[k];
                    ProcessedMorphKey morphKey; // Your struct
                    morphKey.time = (float)aiKey.mTime / anim.ticksPerSecond;
                    morphKey.time = ticksToSeconds(aiKey.mTime, anim.ticksPerSecond);

                    morphKey.targetIndices.assign(aiKey.mValues, aiKey.mValues + aiKey.mNumValuesAndWeights);
                    morphKey.weights.reserve(aiKey.mNumValuesAndWeights);
                    for (uint32_t w = 0; w < aiKey.mNumValuesAndWeights; ++w)
                    {
                        morphKey.weights.push_back(static_cast<float>(aiKey.mWeights[w]));
                    }

                    morphChannel.keys.push_back(std::move(morphKey));
                }

                anim.morphChannels.push_back(std::move(morphChannel));
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

    bool SceneLoader::extractTexturePath(const aiScene* scene, const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath)
    {

        aiString path;
        if (aiMat->GetTexture(type, 0, &path) != AI_SUCCESS)
        {
            return false;
        }

        std::string pathStr(path.C_Str());

        // Check if embedded texture (path is "*0", "*1")
        if (pathStr.rfind('*', 0) == 0)
        {
            int textureIndex = std::stoi(pathStr.substr(1));
            if (textureIndex < 0 || static_cast<unsigned int>(textureIndex) >= scene->mNumTextures)
            {
                outSlot.texturePaths[key] = std::monostate{};
                return false;
            }

            const aiTexture* aiTex = scene->mTextures[textureIndex];
            EmbeddedTextureSource embeddedTex;
            embeddedTex.name = aiTex->mFilename.C_Str();

            // Check if embedded data is compressed
            if (aiTex->mHeight == 0) // Yes it is
            {
                embeddedTex.compressedData = (const uint8_t*)aiTex->pcData;
                embeddedTex.compressedSize = aiTex->mWidth; // Assimp stores size in mWidth
            }
            else
            {
                embeddedTex.rawData = (const uint8_t*)aiTex->pcData;
                embeddedTex.width = aiTex->mWidth;
                embeddedTex.height = aiTex->mHeight;
            }

            // Store the EmbeddedTextureSource struct in our map
            outSlot.texturePaths[key] = embeddedTex;
            return true;
        }


        // For fbx file support - where textures not embedded
        FilePathSource fileSource;
        std::filesystem::path texturePath(pathStr);
        if (texturePath.is_relative())
        {
            fileSource.path = (modelBasePath / texturePath).string();
        }
        else
        {
            fileSource.path = texturePath.string();
        }
        outSlot.texturePaths[key] = fileSource;
        return true;
    }

    ProcessedMaterialSlot SceneLoader::extractMaterialSlot(const aiScene* scene, const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath)
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

        if (!extractTexturePath(scene, aiMat, aiTextureType_BASE_COLOR, texturekeys::BASE_COLOR, slot, modelBasePath))
            extractTexturePath(scene, aiMat, aiTextureType_DIFFUSE, texturekeys::BASE_COLOR, slot, modelBasePath);
        extractTexturePath(scene, aiMat, aiTextureType_METALNESS, texturekeys::METALLIC_ROUGHNESS, slot, modelBasePath);
        if (!extractTexturePath(scene, aiMat, aiTextureType_NORMALS, texturekeys::NORMAL, slot, modelBasePath))
            extractTexturePath(scene, aiMat, aiTextureType_HEIGHT, texturekeys::NORMAL, slot, modelBasePath);
        extractTexturePath(scene, aiMat, aiTextureType_EMISSIVE, texturekeys::EMISSIVE, slot, modelBasePath);
        extractTexturePath(scene, aiMat, aiTextureType_AMBIENT_OCCLUSION, texturekeys::OCCLUSION, slot, modelBasePath);

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
            // glTF has explicit alpha mode - trust it completely
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
            // Non-glTF fallback (FBX, OBJ, etc.)
            // Industry standard (Unity/Unreal): default to Opaque unless there's STRONG evidence
            // of transparency. FBX opacity values are notoriously unreliable - different DCC tools
            // use different semantics and may set opacity < 1.0 even for fully opaque materials.

            slot.alphaMode = AlphaMode::Opaque; // Default like Unity/Unreal

            // Check for opacity texture - strong evidence of intended transparency
            aiString opacityTexPath;
            bool hasOpacityTexture = (aiMat->GetTexture(aiTextureType_OPACITY, 0, &opacityTexPath) == AI_SUCCESS);

            // Check for transparent color (non-black transparent color indicates transparency)
            aiColor3D transparentColor(0.f, 0.f, 0.f);
            bool hasTransparentColor = (aiMat->Get(AI_MATKEY_COLOR_TRANSPARENT, transparentColor) == AI_SUCCESS) &&
                                       (transparentColor.r > 0.01f || transparentColor.g > 0.01f || transparentColor.b > 0.01f);

            if (hasOpacityTexture || hasTransparentColor)
            {
                // Has explicit transparency texture or color - use Blend
                slot.alphaMode = AlphaMode::Blend;
            }
            else if (slot.baseColorFactor.a < 0.1f)
            {
                // Very low opacity (< 10%) - likely intentionally transparent
                // This catches materials explicitly set to be see-through
                slot.alphaMode = AlphaMode::Blend;
            }
            // Otherwise keep Opaque - this matches Unity/Unreal behavior where
            // FBX materials default to opaque and require manual transparency setup
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
                vertex.normal = vec3(0.0f, 1.0f, 0.0f);  // Default up normal when mesh has no normals
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

        const size_t vertexCount = mesh.vertices.size();

        // Skinning
        if (aiMesh->HasBones() && vertexCount > 0)
        {
            mesh.skinning.resize(vertexCount);

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

        // Morphs
        if (aiMesh->mNumAnimMeshes > 0)
        {
            mesh.morphTargets.reserve(aiMesh->mNumAnimMeshes);

            for (uint32_t targetIdx = 0; targetIdx < aiMesh->mNumAnimMeshes; ++targetIdx)
            {
                const aiAnimMesh* aiAnimMesh = aiMesh->mAnimMeshes[targetIdx];
                MorphTargetData target;
                target.name = aiAnimMesh->mName.length > 0 ? aiAnimMesh->mName.C_Str() : ("Morph_" + std::to_string(targetIdx));
                target.deltas.resize(vertexCount);

                const uint32_t limit = std::min<uint32_t>(static_cast<uint32_t>(vertexCount), aiAnimMesh->mNumVertices);
                for (uint32_t v = 0; v < limit; ++v)
                {
                    auto& delta = target.deltas[v];
                    delta.vertexIndex = v;

                    if (aiAnimMesh->mVertices)
                        delta.deltaPosition = AiToVec3(aiAnimMesh->mVertices[v]) - AiToVec3(aiMesh->mVertices[v]);

                    if (aiAnimMesh->mNormals && aiMesh->mNormals)
                        delta.deltaNormal = AiToVec3(aiAnimMesh->mNormals[v]) - AiToVec3(aiMesh->mNormals[v]);

                    if (aiAnimMesh->mTangents && aiMesh->mTangents)
                        delta.deltaTangent = AiToVec3(aiAnimMesh->mTangents[v]) - AiToVec3(aiMesh->mTangents[v]);
                }
                mesh.morphTargets.push_back(std::move(target));
            }
        }

        // Fix winding order: Convert CW meshes to CCW for Vulkan
        FixWindingOrder(mesh);

        // Fix inconsistent normals: Regenerate normals for MIXED meshes
        FixInconsistentNormals(mesh);

        // Diagnostic: Check winding order after fixes
        DiagnoseWindingOrder(mesh, "After fixes");

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
                loadedScene.materials.push_back(extractMaterialSlot(scene, materialPtrs[i], i, baseDir));
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

        // Note: No automatic unit conversion. FBX/GLB files use whatever units the artist exported with.
        // If scale issues occur, either:
        // 1. Fix at export time in the 3D software
        // 2. Add per-asset scale metadata
        // 3. Apply scene-level scale in the engine

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