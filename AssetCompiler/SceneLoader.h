#pragma once
#include <filesystem>
#include <functional>
#include <thread>
#include <optional>

#include "MeshLoader.h"
#include "MeshCompilerData.h"
#include "MaterialLoader.h"
#include "CompileOptions.h"

struct aiScene;
struct aiNode;
//struct aiLight;
//struct aiCamera;

namespace compiler
{
    struct SceneNode
    {
        std::string name;
        mat4 localTransform;
        int32_t meshIndex = -1;
        int32_t parentIndex = -1;
    };

    struct Scene
    {
        std::string name;

        std::vector<SceneNode> nodes;
        std::vector<ProcessedMesh> meshes; // Objects contain direct handles
        std::vector<ProcessedMaterialSlot> materials; // Objects contain direct handles

        // Scene bounds
        vec3 center{ 0 };
        float radius = 0;
        vec3 boundingMin{ FLT_MAX };
        vec3 boundingMax{ -FLT_MAX };
        
        // Statistics
        uint32_t totalMeshes = 0;
        uint32_t totalMaterials = 0;
        uint32_t totalTextures = 0;
    };

    struct SceneLoadData
    {
        std::optional<Scene> scene;
        std::vector<std::string> errors;
    };

    class SceneLoader
    {
    public:
        using ProgressCallback = std::function<void(float progress, const std::string& status)>;

        SceneLoader() = default;

        SceneLoadData loadScene(const std::filesystem::path& path, const MeshOptions& meshOptions);

    private:

        static void reportProgress(const ProgressCallback& callback, float progress, const std::string& status);

        std::unique_ptr<const aiScene, void(*)(const aiScene*)> importScene(const std::filesystem::path& path) const;


        std::vector<const aiMesh*> collectMeshPointers(const aiScene* scene, const MeshOptions& options);
        void extractVertices(const aiMesh* aiMesh, std::vector<Vertex>& vertices, const MeshOptions& options);
        ProcessedMesh extractMesh(const aiMesh* aiMesh, uint32_t meshIndex, const MeshOptions& options);


        ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath);

        void extractNodesForCompiler(
            const aiNode* node,
            const int32_t parentIdx,
            const aiScene* scene,
            std::vector<SceneNode>& outNodes) const;

        //void resolveSceneObjectHandles(std::vector<SceneObject>& objects, const std::vector<MeshHandle>& meshHandles, const std::vector<MaterialHandle>& materialHandles) const;

        //void calculateBounds(Scene& scene, std::vector<ProcessedMesh>& meshes);
    };
}
