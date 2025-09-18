#pragma once
#include <filesystem>
#include <functional>
#include <thread>

#include "material_loader.h"
#include "mesh_loader.h"
#include "texture_loader.h"

#include "assets/core/import_config.h"
#include "graphics/scene.h"

struct aiScene;
struct aiNode;
struct aiLight;
struct aiCamera;

namespace AssetLoading
{
  struct Scene
  {
    std::string name;
    // Scene elements with resolved handles
    std::vector<SceneLight> lights;
    std::vector<SceneCamera> cameras;
    std::vector<SceneObject> objects; // Objects contain direct handles
    // Scene bounds
    vec3 center{0};
    float radius = 0;
    vec3 boundingMin{FLT_MAX};
    vec3 boundingMax{-FLT_MAX};
    // Statistics
    uint32_t totalMeshes = 0;
    uint32_t totalMaterials = 0;
    uint32_t totalTextures = 0;
  };

  struct SceneLoadResult
  {
    Scene scene;
    bool success = true;
    std::vector<std::string> warnings;

    struct Stats
    {
      float totalTimeMs = 0.0f;
      size_t meshesLoaded = 0;
      size_t materialsLoaded = 0;
      size_t texturesLoaded = 0;
    } stats;
  };

  class SceneLoader
  {
    public:
      using ProgressCallback = std::function<void(float progress, const std::string& status)>;

      explicit SceneLoader(AssetSystem& assetSystem);

      SceneLoadResult loadScene(const std::filesystem::path& path, const LoadingConfig& config = LoadingConfig::createBalanced(), ProgressCallback onProgress = nullptr);

    private:
      AssetSystem& m_assetSystem;

      // --- Helper Methods (const because they don't modify SceneLoader state) ---
      static void reportProgress(const ProgressCallback& callback, float progress, const std::string& status);

      std::unique_ptr<const aiScene, void(*)(const aiScene*)> importScene(const std::filesystem::path& path) const;

      SceneLight extractSingleLight(const aiLight* aiLight, uint32_t index) const;

      SceneCamera extractSingleCamera(const aiCamera* aiCamera, uint32_t index) const;

      void extractObjectsFromNode(const aiNode* node, const mat4& parentTransform, const aiScene* scene, std::vector<SceneObject>& objects) const;

      void resolveSceneObjectHandles(std::vector<SceneObject>& objects, const std::vector<MeshHandle>& meshHandles, const std::vector<MaterialHandle>& materialHandles) const;

      void calculateBounds(Scene& scene, AssetSystem& assetSystem) const;

      bool isValidScene(const aiScene* scene) const;
  };
}
