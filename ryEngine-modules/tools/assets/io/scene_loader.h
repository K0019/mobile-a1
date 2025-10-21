#pragma once
#include <filesystem>
#include <functional>
#pragma once

#include "material_loader.h"
#include "mesh_loader.h"
#include "texture_loader.h"
#include "import_config.h"

struct aiScene;
struct aiNode;
struct aiLight;
struct aiCamera;

namespace Resource
{
  class SceneLoader
  {
    public:
      using ProgressCallback = std::function<void(float progress, const std::string& status)>;

      explicit SceneLoader(ResourceManager& assetSystem);

      SceneLoadResult loadScene(const std::filesystem::path& path, const LoadingConfig& config = LoadingConfig::createBalanced(), ProgressCallback onProgress = nullptr);

    private:
      ResourceManager& m_assetSystem;

      static void reportProgress(const ProgressCallback& callback, float progress, const std::string& status);

      std::unique_ptr<const aiScene, void(*)(const aiScene*)> importScene(const std::filesystem::path& path) const;

      SceneLight extractSingleLight(const aiLight* aiLight, uint32_t index) const;

      SceneCamera extractSingleCamera(const aiCamera* aiCamera, uint32_t index) const;

      void extractObjectsFromNode(const aiNode* node, const mat4& parentTransform, const aiScene* scene, std::vector<SceneObject>& objects) const;

      void resolveSceneObjectHandles(std::vector<SceneObject>& objects, const std::vector<MeshHandle>& meshHandles, const std::vector<MaterialHandle>& materialHandles) const;

      void calculateBounds(Scene& scene, ResourceManager& assetSystem) const;

      bool isValidScene(const aiScene* scene) const;
  };
} // namespace AssetLoading
