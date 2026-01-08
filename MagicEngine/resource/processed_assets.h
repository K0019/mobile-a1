#pragma once
#include <array>
#include <limits>
#include "math/utils_math.h"
#include "resource/animation_ids.h"
#include "resource/font_types.h"
#include "renderer/scene.h"
#ifndef NOMINMAX
#define NOMINMAX
#endif
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

  static constexpr uint32_t INVALID_BONE_INDEX = static_cast<uint32_t>(-1);

  struct SkinningData
  {
    std::array<uint32_t, 4> boneIndices{INVALID_BONE_INDEX, INVALID_BONE_INDEX, INVALID_BONE_INDEX, INVALID_BONE_INDEX};
    std::array<float, 4> weights{0.0f, 0.0f, 0.0f, 0.0f};

    bool isStatic() const { return boneIndices[0] == INVALID_BONE_INDEX; }
  };

  static constexpr uint32_t INVALID_JOINT_INDEX = INVALID_BONE_INDEX;

  struct ProcessedSkeleton
  {
    std::vector<uint32_t> parentIndices;
    std::vector<mat4> inverseBindMatrices;
    std::vector<mat4> bindPoseMatrices;
    std::vector<std::string> jointNames;

    bool empty() const { return parentIndices.empty(); }
  };

  struct MorphTargetVertexDelta
  {
    uint32_t vertexIndex = 0;
    vec3 deltaPosition{0.0f};
    vec3 deltaNormal{0.0f};
    vec3 deltaTangent{0.0f};
  };

  struct MorphTargetData
  {
    std::string name;
    std::vector<MorphTargetVertexDelta> deltas;

    bool empty() const { return deltas.empty(); }
  };

  struct ProcessedAnimationVectorKey
  {
    float time = 0.0f;
    vec3 value{0.0f};
  };

  struct ProcessedAnimationQuatKey
  {
    float time = 0.0f;
    glm::quat value{1.0f, 0.0f, 0.0f, 0.0f};
  };

  struct ProcessedAnimationChannel
  {
    std::string nodeName;
    std::vector<ProcessedAnimationVectorKey> translationKeys;
    std::vector<ProcessedAnimationQuatKey> rotationKeys;
    std::vector<ProcessedAnimationVectorKey> scaleKeys;
  };

  struct ProcessedMorphKey
  {
    float time = 0.0f;
    std::vector<uint32_t> targetIndices;
    std::vector<float> weights;
  };

  struct ProcessedMorphChannel
  {
    uint32_t meshIndex = UINT32_MAX;
    std::string meshName;
    std::vector<ProcessedMorphKey> keys;
  };

  struct ProcessedAnimationClip
  {
    std::string name;
    float duration = 0.0f;
    float ticksPerSecond = 0.0f;
    std::vector<ProcessedAnimationChannel> skeletalChannels;
    std::vector<ProcessedMorphChannel> morphChannels;
  };

  struct ProcessedMesh
  {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::string name;
    uint32_t materialIndex = UINT32_MAX;
    vec4 bounds{ 0, 0, 0, 1 }; // x,y,z = center, w = radius
    std::vector<SkinningData> skinning;
    ProcessedSkeleton skeleton;
    std::vector<MorphTargetData> morphTargets;
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
    std::vector<ClipId> animationClips;
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

  class TextureCacheKeyGenerator
  {
  public:
    static std::string generateKey(const TextureDataSource& source)
    {
      return std::visit([](const auto& src) -> std::string
      {
        using T = std::decay_t<decltype(src)>;
        if constexpr (std::is_same_v<T, std::monostate>)
        {
          return "";
        }
        else if constexpr (std::is_same_v<T, FilePathSource>)
        {
            //return std::filesystem::canonical(src.path).string(); // Canonical path
            return src.path; // because of vfs, should already be canonical.
        }
        else if constexpr (std::is_same_v<T, EmbeddedMemorySource>)
        {
          return src.scenePath + "|" + src.identifier;
        }
      }, source);
    }
  };

  struct ProcessedFont
  {
    std::string name;
    std::vector<uint8_t> fontFileData;
    FontBuildSettings buildSettings;
    std::filesystem::path sourceFile;
    std::vector<FontMergeSource> mergeSources;

    bool isValid() const
    {
      if (fontFileData.empty() || name.empty())
      {
        return false;
      }
      const bool mergesValid = std::all_of(mergeSources.begin(), mergeSources.end(), [](const FontMergeSource& merge)
      {
        return merge.isValid();
      });
      return mergesValid;
    }
  };
} // namespace AssetLoading
