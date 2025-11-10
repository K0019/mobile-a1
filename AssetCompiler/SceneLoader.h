/******************************************************************************/
/*!
\file   SceneLoader.h
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
#pragma once
#include "CompilerMath.h"
#include "CompileOptions.h"
#include "CompilerTypes.h"

#include <assimp/material.h>

#include <filesystem>
#include <functional>
#include <thread>
#include <optional>
#include <map>

struct aiScene;
struct aiMaterial;
struct aiNode;
struct aiMesh;
namespace compiler
{

    class SceneLoader
    {
    public:
        using ProgressCallback = std::function<void(float progress, const std::string& status)>;

        SceneLoader() = default;

        SceneLoadData loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions);

    private:
        static void reportProgress(const ProgressCallback& callback, float progress, const std::string& status);

        std::unique_ptr<const aiScene, void(*)(const aiScene*)> importScene(const std::filesystem::path& path) const;

        // Anims
        void extractSkeleton(const aiScene* scene, Scene& outScene);
        void buildBoneHierarchy(const aiNode* node, int32_t parentBoneIndex, ProcessedSkeleton& skeleton);
        void extractAnimations(const aiScene* scene, Scene& outScene);

        // Meshes
        std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const MeshOptions& options);
        void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const MeshOptions& options);
        ProcessedMesh extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const MeshOptions& options, const ProcessedSkeleton& skeleton);

        // Materials
        std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene);
        ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath);
        bool extractTexturePath(const aiMaterial* aiMat, aiTextureType type, const std::string& key, ProcessedMaterialSlot& outSlot, const std::filesystem::path& modelBasePath);

        // Nodes
        void extractNodesForCompiler(
            const aiNode* node,
            const int32_t parentIdx,
            const aiScene* scene,
            std::vector<SceneNode>& outNodes) const;
    };
}


// Self notes:
// https://ogldev.org/www/tutorial38/tutorial38.html describes the file format of assimp data structures
    // A Mesh contains a list of bones. these bones together forms a skeleton.
    // - A bone describes which vertices it affects. we use this info to populate Vertex.boneIndices and .boneWeights
    //   - We use Vertex.boneIndices as in index into ProcessedSkeleton.bones[] to find the bone which affects the vertex.
    // 
    // An aiScene also contains a list of aiAnimations.
    //  - An aiAnimation object represents a sequence of animation frames (the entire animation)
    //  - Each aiAnimation contains timing information, as well as an array of Channels[].
    //    - Each channel is just a bone, with all of its animation frames. 
    //    - Each channel contains a name which must match one of the nodes in the hierarchy.
    // A ProcessedAnimation describes an animation "file", and each ProcessedBoneChannel describes the movement of each bone during the animation.

// Morph animtion
    // Again, a morph animation will have a vector of channels, with each channel describing