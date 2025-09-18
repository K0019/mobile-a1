#pragma once
#include <vector>
#include "assets/core/asset_types.h"
#include "assets/core/import_config.h"

struct aiScene;
struct aiMaterial;

namespace AssetLoading
{
  struct ProcessedMaterial
  {
    std::string name;

    // Core PBR properties
    vec4 baseColorFactor = vec4(1.0f);
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    vec3 emissiveFactor = vec3(0.0f);
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    float alphaCutoff = 0.5f;

    // Texture properties with transforms
    MaterialTexture baseColorTexture;
    MaterialTexture metallicRoughnessTexture;
    MaterialTexture normalTexture;
    MaterialTexture emissiveTexture;
    MaterialTexture occlusionTexture;

    // Material properties
    AlphaMode alphaMode = AlphaMode::Opaque;
    uint32_t materialTypeFlags = MaterialType_MetallicRoughness;
    uint32_t flags = sMaterialFlags_CastShadow | sMaterialFlags_ReceiveShadow;
    bool doubleSided = false;

    // For texture loader compatibility
    std::vector<TextureDataSource> textures;

    bool isTransparent() const
    {
      return alphaMode == AlphaMode::Blend || baseColorFactor.a < 1.0f;
    }

    bool isAlphaTested() const
    {
      return alphaMode == AlphaMode::Mask;
    }
  };

  namespace MaterialLoading
  {
    // Single material extraction - fully parallelizable
    ProcessedMaterial extractMaterial(const aiMaterial* aiMat, uint32_t materialIndex, const std::string& scenePath, const std::string& baseDir, const aiScene* scene);

    // Collection utilities
    std::vector<const aiMaterial*> collectMaterialPointers(const aiScene* scene, const LoadingConfig& config);

    // Conversion and utility functions
    Material convertToStoredMaterial(const ProcessedMaterial& processed);

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
  }
}
