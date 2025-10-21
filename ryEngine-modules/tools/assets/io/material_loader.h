#pragma once
#include <vector>

#include <resource/processed_assets.h>
#include "resource/resource_types.h"
#include "import_config.h"

struct aiScene;
struct aiMaterial;

namespace Resource
{
  namespace MaterialLoading
  {
    // Single material extraction - fully parallelizable
    ProcessedMaterial extractMaterial(const aiMaterial* aiMat, uint32_t materialIndex, const std::string& scenePath, const std::string& baseDir, const aiScene* scene);

    // Collection utilities
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene, const LoadingConfig& config);

    bool isValidMaterial(const ProcessedMaterial& material);

    std::string generateMaterialCacheKey(const ProcessedMaterial& material);

    std::vector<ProcessedMaterial> deduplicateMaterials(std::vector<ProcessedMaterial> materials, float tolerance = 0.001f);

    // Core extraction implementations (internal use)
    namespace Detail
    {
      MaterialTexture extractTextureWithTransform(const aiMaterial* aiMat, unsigned int type, const std::string& scenePath, const std::string& baseDir, const aiScene* scene);

      TextureDataSource extractTextureSource(const std::string& texturePath, const std::string& scenePath, const std::string& baseDir, const aiScene* scene);

      void populateTexturesVector(ProcessedMaterial& material);

      bool compareMaterials(const ProcessedMaterial& a, const ProcessedMaterial& b, float tolerance);
    }
  } // namespace MaterialLoading
}   // namespace AssetLoading
