/******************************************************************************/
/*!
\file   FbxLoader.h
\brief  FBX scene loader using ufbx library. Replaces Assimp for FBX files.
*/
/******************************************************************************/
#pragma once
#include "CompilerMath.h"
#include "CompileOptions.h"
#include "CompilerTypes.h"

#include <filesystem>

struct ufbx_scene;
struct ufbx_mesh;
struct ufbx_mesh_part;
struct ufbx_node;
struct ufbx_material;
struct ufbx_skin_deformer;
struct ufbx_baked_anim;

namespace compiler
{
    class FbxLoader
    {
    public:
        SceneLoadData loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions);

    private:
        // Scene import
        struct UfbxSceneDeleter { void operator()(ufbx_scene* s) const; };
        using UfbxScenePtr = std::unique_ptr<ufbx_scene, UfbxSceneDeleter>;
        UfbxScenePtr importScene(const std::filesystem::path& path) const;

        // Skeleton
        void extractSkeleton(const ufbx_scene* scene, Scene& outScene);

        // Animations
        void extractAnimations(const ufbx_scene* scene, Scene& outScene);

        // Meshes
        ProcessedMesh extractMesh(const ufbx_scene* scene, const ufbx_mesh* mesh,
                                  const ufbx_node* meshNode,  // Node for geometric transform
                                  uint32_t meshIndex, const MeshOptions& options,
                                  const ProcessedSkeleton& skeleton,
                                  const ufbx_mesh_part* materialPart = nullptr);

        // Materials
        ProcessedMaterialSlot extractMaterial(const ufbx_scene* scene, const ufbx_material* mat,
                                              uint32_t materialIndex,
                                              const std::filesystem::path& modelBasePath);

        // Nodes
        void extractNodes(const ufbx_node* node, int32_t parentIdx,
                          std::vector<SceneNode>& outNodes) const;
    };
}
