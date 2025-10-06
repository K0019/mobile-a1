#pragma once
#include "MeshCompilerData.h"
#include "CompileOptions.h"

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
    // ----- Materials specific -----
    // Copied same format as ryan for compatability
    enum class AlphaMode : uint32_t
    {
        Opaque = 0,
        Mask = 1,
        Blend = 2
    };
    enum MaterialType : uint32_t
    {
        INVALID = 0,
        METALLIC_ROUGHNESS = 0x1,
        SPECULAR_GLOSSINESS = 0x2,
    };
    enum MaterialFlags : uint32_t
    {
        // Alpha mode (lower 2 bits)
        ALPHA_MODE_MASK = 0x3,   // 2 bits for masking
        ALPHA_MODE_OPAQUE = 0x0,   // 00
        ALPHA_MODE_MASK_TEST = 0x1,  // 01 (renamed to avoid conflict)
        ALPHA_MODE_BLEND = 0x2,   // 10

        // Material properties
        DOUBLE_SIDED = 0x4,   // Bit 2
        UNLIT = 0x8,   // Bit 3
        CAST_SHADOW = 0x10,  // Bit 4  
        RECEIVE_SHADOW = 0x20,  // Bit 5

        // Default flags
        DEFAULT_FLAGS = CAST_SHADOW | RECEIVE_SHADOW
    };
    struct ProcessedMaterialSlot
    {
        std::string name;
        uint32_t originalIndex;

        // Core PBR properties
        vec4 baseColorFactor = vec4(1.0f);
        float metallicFactor = 1.0f;
        float roughnessFactor = 1.0f;
        vec3 emissiveFactor = vec3(0.0f);
        float normalScale = 1.0f;
        float occlusionStrength = 1.0f;
        float alphaCutoff = 0.5f;

        // Material properties
        AlphaMode alphaMode = AlphaMode::Opaque;
        uint32_t materialTypeFlags = 0;
        uint32_t flags = 0;

        // Key: Texture type (e.g., "baseColor", "normal")
        // Value: The SOURCE filepath of the texture
        std::map<std::string, std::filesystem::path> texturePaths;
    };

    // ----- Mesh -----
    struct ProcessedMesh
    {
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        std::string name;
        uint32_t materialIndex = UINT32_MAX;
        vec4 bounds{ 0, 0, 0, 1 }; // x,y,z = center, w = radius
    };

    // ----- Scene -----
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

        std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene);
        ProcessedMaterialSlot extractMaterialSlot(const aiMaterial* aiMat, uint32_t materialIndex, const std::filesystem::path& modelBasePath);

        void extractNodesForCompiler(
            const aiNode* node,
            const int32_t parentIdx,
            const aiScene* scene,
            std::vector<SceneNode>& outNodes) const;
    };
}
