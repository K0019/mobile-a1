#pragma once
#include <atomic>
#include <filesystem>
#include <array>
#include <vector>
#include <variant>
#include "resource_handle.h"
#include "renderer/gpu_data.h"
#include "math/utils_math.h"
#include "renderer/gfx_interface.h"
#include "renderer/render_feature.h"
#include "resource/animation_ids.h"
#include "resource/font_types.h"

// Resource limits for logging/sanity checks only
// Actual memory is managed dynamically by ChunkedMeshStorage and GfxMaterialSystem
namespace ResourceLimits
{
#ifdef __ANDROID__
  static constexpr size_t MAX_VERTICES = 2'000'000;
  static constexpr size_t MAX_INDICES = 6'000'000;
  static constexpr size_t MAX_MATERIALS = 10'000;
#else
  static constexpr size_t MAX_VERTICES = 5'000'000;
  static constexpr size_t MAX_INDICES = 12'000'000;
  static constexpr size_t MAX_MATERIALS = 10'000;
#endif
} // namespace ResourceLimits

struct LoadOptions
{
  bool loadAllMeshes = true; // Load all meshes vs first mesh only
  bool loadMaterials = true; // Load materials or use defaults
  bool loadTextures = true; // Load referenced textures
  bool generateMissingUVs = true; // Generate UVs if missing
  bool optimizeMeshes = true; // Apply meshoptimizer
  size_t meshLimit = 100; // Safety limit for mesh count
  size_t textureLimit = 200; // Safety limit for texture count
  std::string basePath; // Override texture search path
};

struct aiScene;

struct FilePathSource
{
  //std::filesystem::path path;
  std::string path;

  bool operator<(const FilePathSource& other) const
  {
    return path < other.path;
  }

  bool operator==(const FilePathSource& other) const
  {
    return path == other.path;
  }
};

struct EmbeddedMemorySource
{
  const aiScene* scene = nullptr; // Pointer to the scene containing the texture
  std::string identifier; // e.g., "*0", "*1"
  std::string scenePath; // The path of the parent scene file, for unique caching
  bool operator<(const EmbeddedMemorySource& other) const
  {
    if (scenePath != other.scenePath) return scenePath < other.scenePath;
    return identifier < other.identifier;
  }

  bool operator==(const EmbeddedMemorySource& other) const
  {
    if (!scene || !other.scene) return false;
    return scene == other.scene && identifier == other.identifier && scenePath == other.scenePath;
  }
};

using TextureDataSource = std::variant<std::monostate, FilePathSource, EmbeddedMemorySource>;
template <typename Tag>
struct ResourceTraits;

struct MeshAsset
{
};

struct MeshMorphTargetInfo
{
  std::string name;
  uint32_t morphTargetIndex = 0; // Index into mesh-local morph list
};

template <>
struct ResourceTraits<MeshAsset>
{
  struct HotData
  {
    uint32_t vertexCount;
    uint32_t indexCount;
    vec4 bounds;
    struct AnimationOffsets
    {
      uint32_t skinningByteOffset = UINT32_MAX;
      uint32_t morphDeltaByteOffset = UINT32_MAX;
      uint32_t morphDeltaCount = 0;
      uint32_t morphVertexBaseOffset = UINT32_MAX;
      uint32_t morphVertexCountOffset = UINT32_MAX;
      uint32_t morphTargetCount = 0;
      uint32_t jointCount = 0;
    } animation;

    // GfxRenderer integration
    bool hasGfxMesh = false;
    uint32_t gfxMeshIndex = 0;
    uint32_t gfxMeshGeneration = 0;

    HotData() = default;

    HotData(uint32_t vCount, uint32_t iCount, const vec4& b)
      : vertexCount(vCount), indexCount(iCount), bounds(b)
    {
    }
  };

  struct ColdData
  {
    std::string sourceFile;
    std::string meshName; // Name from file (e.g., "Cube.001")
    // Optimization results
    float vertexCacheOptimization{0.0f}; // ACMR improvement ratio
    float overdrawOptimization{0.0f}; // Overdraw reduction ratio
    bool wasOptimized{false};
    // Animation metadata
    std::vector<uint32_t> jointParentIndices;
    std::vector<mat4> jointInverseBindMatrices;
    std::vector<mat4> jointBindPoseMatrices;
    std::vector<std::string> jointNames;
    std::vector<MeshMorphTargetInfo> morphTargets;
    Resource::SkeletonId skeletonId = Resource::INVALID_SKELETON_ID;
    Resource::MorphSetId morphSetId = Resource::INVALID_MORPH_SET_ID;

    ColdData() = default;

    explicit ColdData(std::string file, std::string name = "") : sourceFile(std::move(file)), meshName(std::move(name))
    {
    }
  };

  static constexpr std::string_view supported_extensions[] = {".gltf", ".glb", ".obj", ".fbx"};
};

using MeshHandle = Handle<MeshAsset>;
using MeshGPUData = ResourceTraits<MeshAsset>::HotData;

struct TextureAsset
{
};

template <>
struct ResourceTraits<TextureAsset>
{
  struct HotData
  {
    // GfxRenderer integration
    // Atomic for thread-safe publication from async upload thread to render thread.
    // Use acquire/release ordering: writer stores with release after setting index/gen,
    // reader loads with acquire before reading index/gen.
    std::atomic<bool> hasGfxTexture{false};
    uint32_t gfxTextureIndex = 0;
    uint32_t gfxTextureGeneration = 0;

    HotData() = default;

    // Explicit copy/move since std::atomic is not copyable
    HotData(const HotData& other)
      : hasGfxTexture(other.hasGfxTexture.load(std::memory_order_relaxed))
      , gfxTextureIndex(other.gfxTextureIndex)
      , gfxTextureGeneration(other.gfxTextureGeneration) {}

    HotData& operator=(const HotData& other) {
      hasGfxTexture.store(other.hasGfxTexture.load(std::memory_order_relaxed), std::memory_order_relaxed);
      gfxTextureIndex = other.gfxTextureIndex;
      gfxTextureGeneration = other.gfxTextureGeneration;
      return *this;
    }

    HotData(HotData&& other) noexcept
      : hasGfxTexture(other.hasGfxTexture.load(std::memory_order_relaxed))
      , gfxTextureIndex(other.gfxTextureIndex)
      , gfxTextureGeneration(other.gfxTextureGeneration) {}

    HotData& operator=(HotData&& other) noexcept {
      hasGfxTexture.store(other.hasGfxTexture.load(std::memory_order_relaxed), std::memory_order_relaxed);
      gfxTextureIndex = other.gfxTextureIndex;
      gfxTextureGeneration = other.gfxTextureGeneration;
      return *this;
    }
  };

  struct ColdData
  {
    gfx::TextureMetadata textureDesc;
    // File properties
    std::string sourceFile; // Original file path or embedded identifier
    std::string cacheKey; // Unique key for caching
    bool isSRGB{true}; // Color space
    bool isCompressed{false}; // KTX vs standard image
    size_t originalFileSize{0}; // File size in bytes
    size_t uncompressedSize{0}; // Raw texture data size
    ColdData() = default;

    explicit ColdData(std::string file, bool srgb = true) : sourceFile(std::move(file)), isSRGB(srgb)
    {
    }
  };

  static constexpr std::string_view supported_extensions[] = {
    ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".dds", ".ktx", ".ktx2", ".hdr", ".exr"
  };
};

using TextureHandle = Handle<TextureAsset>;

struct FontAsset
{
};

template <>
struct ResourceTraits<FontAsset>
{
  struct HotData
  {
    TextureHandle atlasTexture;
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineGap = 0.0f;
    float fontSize = 0.0f;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    Resource::FontCPUData cpuData;
  };

  struct ColdData
  {
    std::vector<uint8_t> fontBinary;
    std::string cacheKey;
    std::vector<Resource::FontMergeSource> mergeSources;
  };

  static constexpr std::string_view supported_extensions[] = {".ttf", ".otf"};
};

using FontHandle = Handle<FontAsset>;

enum MaterialFlags : uint32_t
{
  // Alpha mode (lower 2 bits)
  ALPHA_MODE_MASK = 0x3, // 2 bits for masking
  ALPHA_MODE_OPAQUE = 0x0, // 00
  ALPHA_MODE_MASK_TEST = 0x1, // 01 (renamed to avoid conflict)
  ALPHA_MODE_BLEND = 0x2, // 10
  // Material properties
  DOUBLE_SIDED = 0x4, // Bit 2
  UNLIT = 0x8, // Bit 3
  CAST_SHADOW = 0x10, // Bit 4  
  RECEIVE_SHADOW = 0x20, // Bit 5
  // Default flags
  DEFAULT_FLAGS = CAST_SHADOW | RECEIVE_SHADOW
};

// Simplified material type - remove redundancy with flags
enum MaterialType : uint32_t
{
  INVALID = 0,
  METALLIC_ROUGHNESS = 0x1,
  SPECULAR_GLOSSINESS = 0x2,
};

// Keep AlphaMode enum for clarity in material struct
enum class AlphaMode : uint32_t
{
  Opaque = 0,
  Mask = 1,
  Blend = 2
};

enum class TextureDependencyType : uint8_t
{
  BaseColor = 0,
  Normal = 1,
  MetallicRoughness = 2,
  Emissive = 3,
  Occlusion = 4
};

enum class RenderQueue : uint8_t
{
  Opaque = 0,
  Transparent = 1
};

struct MaterialTexture
{
  TextureDataSource source;
  uint32_t uvSet = 0;
  vec2 uvScale = vec2(1.0f);
  vec2 uvOffset = vec2(0.0f);
  bool isSRGB = false;  // True for base color, emissive; false for normal, metallic-roughness, occlusion

  bool hasTexture() const
  {
    return !std::holds_alternative<std::monostate>(source);
  }
};

struct Material
{
  std::string name;
  // Core PBR properties
  vec4 baseColorFactor = vec4(1.0f);
  float metallicFactor = 1.0f;
  float roughnessFactor = 1.0f;
  vec3 emissiveFactor = vec3(0.0f); // Changed from vec4 to vec3
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
};

struct MaterialAsset
{
};

template <>
struct ResourceTraits<MaterialAsset>
{
  struct HotData
  {
    RenderQueue renderQueue = RenderQueue::Opaque;

    // GfxRenderer integration
    bool hasGfxMaterial = false;
    uint32_t gfxMaterialIndex = 0;
    uint32_t gfxMaterialGeneration = 0;

    HotData() = default;

    explicit HotData(RenderQueue queue) : renderQueue(queue)
    {
    }
  };

  struct ColdData
  {
    Material material;
    std::string sourceFile;
    size_t materialIndex = 0;
    bool wasLoadedFromFile = false;

    ColdData() = default;

    explicit ColdData(Material mat) : material(std::move(mat))
    {
    }

    ColdData(Material mat, std::string file, size_t idx) : material(std::move(mat)), sourceFile(std::move(file)),
                                                           materialIndex(idx), wasLoadedFromFile(true)
    {
    }
  };

  static constexpr std::array supported_extensions = {".mtl", ".mat", ".json"};
};

using MaterialHandle = Handle<MaterialAsset>;

struct TextureDependency
{
  MaterialHandle materialHandle;
  TextureDependencyType dependencyType;

  bool operator==(const TextureDependency& other) const
  {
    return materialHandle == other.materialHandle && dependencyType == other.dependencyType;
  }
};

template <typename T>concept IsValidResourceTrait = requires
{
  typename ResourceTraits<T>; typename T::HotData; typename T::ColdData;
  // Check for the existence of the static supported_extensions member
  // and ensure it's iterable using std::begin and std::end.
  {
    std::begin(T::supported_extensions)
  }; {
    std::end(T::supported_extensions)
  };
};
template <typename T>concept IsResource = IsValidResourceTrait<ResourceTraits<T>>;
