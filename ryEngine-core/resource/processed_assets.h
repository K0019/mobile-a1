#pragma once
#include "graphics/scene.h"

namespace Resource
{
  struct ProcessedTexture
  {
    TextureDataSource source;
    std::string name;
    std::vector<uint8_t> data;
    vk::TextureDesc textureDesc;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    bool sRGB = true;
    size_t originalFileSize = 0;
  };

  struct ProcessedMesh
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
    uint32_t materialIndex = UINT32_MAX;
    vec4 bounds{ 0, 0, 0, 1 }; // x,y,z = center, w = radius
  };

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
    uint32_t materialTypeFlags = 0;
    uint32_t flags = 0;

    // For texture loader compatibility
    std::vector<TextureDataSource> textures;

    bool isTransparent() const
    {
      return alphaMode == AlphaMode::Blend;
    }

    AlphaMode getAlphaModeFromFlags() const
    {
    return static_cast<AlphaMode>(flags & ALPHA_MODE_MASK);
    }
  };

  inline Material convertToStoredMaterial(const ProcessedMaterial& processed)
  {
    Material material;

    material.name = processed.name;
    material.baseColorFactor = processed.baseColorFactor;
    material.metallicFactor = processed.metallicFactor;
    material.roughnessFactor = processed.roughnessFactor;
    material.emissiveFactor = processed.emissiveFactor;
    material.normalScale = processed.normalScale;
    material.occlusionStrength = processed.occlusionStrength;
    material.alphaCutoff = processed.alphaCutoff;

    material.baseColorTexture = processed.baseColorTexture;
    material.metallicRoughnessTexture = processed.metallicRoughnessTexture;
    material.normalTexture = processed.normalTexture;
    material.emissiveTexture = processed.emissiveTexture;
    material.occlusionTexture = processed.occlusionTexture;

    material.alphaMode = processed.alphaMode;
    material.materialTypeFlags = processed.materialTypeFlags;
    material.flags = processed.flags;

    return material;
  }

  struct Scene
  {
    std::string name;
    // Scene elements with resolved handles
    std::vector<SceneLight> lights;
    std::vector<SceneCamera> cameras;
    std::vector<SceneObject> objects; // Objects contain direct handles
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

  class TextureCacheKeyGenerator
  {
    public:
    static std::string generateKey(const TextureDataSource& source)
    {
      return std::visit([](const auto& src) -> std::string
      {
        using T = std::decay_t<decltype(src)>;
        if constexpr(std::is_same_v<T, std::monostate>)
        {
          return "";
        }
        else if constexpr(std::is_same_v<T, FilePathSource>)
        {
            //return std::filesystem::canonical(src.path).string(); // Canonical path
            return src.path; // because of vfs, should already be canonical.
        }
        else if constexpr(std::is_same_v<T, EmbeddedMemorySource>)
        {
          return src.scenePath + "|" + src.identifier;
        }
      },
                        source);
    }
  };
} // namespace AssetLoading
