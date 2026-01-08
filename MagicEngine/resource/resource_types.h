#pragma once
#include <filesystem>
#include <array>
#include <vector>
#include <offsetAllocator.hpp>
#include <variant>
#include "resource_handle.h"
#include "renderer/gpu_data.h"
#include "math/utils_math.h"
#include "renderer/interface.h"
#include "renderer/render_feature.h"
#include "resource/animation_ids.h"
#include "resource/font_types.h"

namespace ResourceLimits
{
#ifdef __ANDROID__
  // Reduced limits for Android to fit in mobile GPU memory
  static constexpr size_t MAX_VERTICES = 2'000'000;  // 2M vertices
  static constexpr size_t MAX_INDICES = 6'000'000;   // 6M indices
  static constexpr size_t MAX_MATERIALS = 10'000;    // 10K materials
  static constexpr size_t MAX_TEXTURES = 2'000;      // 2K textures
  static constexpr size_t MAX_MESHES = 20'000;       // 20K meshes
  static constexpr size_t MAX_MORPH_TARGET_VERTICES = MAX_VERTICES; // 1x multiplier for mobile
#else
  // lower the budget slightly.
  static constexpr size_t MAX_VERTICES = 5'000'000; // 10M vertices
  static constexpr size_t MAX_INDICES = 12'000'000;  // 30M indices
  static constexpr size_t MAX_MATERIALS = 10'000;    // 50K materials
  static constexpr size_t MAX_TEXTURES = 2'000;     // 10K textures
  static constexpr size_t MAX_MESHES = 30'000;      // 100K meshes
  static constexpr size_t MAX_MORPH_TARGET_VERTICES = MAX_VERTICES * 4;
#endif

  static constexpr size_t VERTEX_BUFFER_SIZE = MAX_VERTICES * sizeof(CompressedVertex);
  static constexpr size_t INDEX_BUFFER_SIZE = MAX_INDICES * sizeof(uint32_t);
  static constexpr size_t MATERIAL_BUFFER_SIZE = MAX_MATERIALS * sizeof(MaterialData);
  static constexpr size_t MESH_DECOMPRESSION_BUFFER_SIZE = MAX_MESHES * sizeof(MeshDecompressionData);
  static constexpr size_t SKINNING_BUFFER_SIZE = MAX_VERTICES * sizeof(GPUSkinningData);
  static constexpr size_t MORPH_DELTA_BUFFER_SIZE = MAX_MORPH_TARGET_VERTICES * sizeof(GPUMorphDelta);
  static constexpr size_t MORPH_VERTEX_BASE_BUFFER_SIZE = MAX_VERTICES * sizeof(uint32_t);
  static constexpr size_t MORPH_VERTEX_COUNT_BUFFER_SIZE = MAX_VERTICES * sizeof(uint32_t);
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
    uint32_t vertexByteOffset; // GPU buffer offset
    uint32_t indexByteOffset; // GPU buffer offset
    uint32_t vertexCount;
    uint32_t indexCount;
    vec4 bounds;
    uint32_t decompressionByteOffset; // Offset into mesh decompression buffer
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

    HotData() = default;

    HotData(uint32_t vOffset, uint32_t iOffset, uint32_t vCount, uint32_t iCount, const vec4& b,
            uint32_t decompOffset = 0)
      : vertexByteOffset(vOffset), indexByteOffset(iOffset), vertexCount(vCount), indexCount(iCount), bounds(b),
        decompressionByteOffset(decompOffset)
    {
    }
  };

  struct ColdData
  {
    // Loading metadata
    OffsetAllocator::Allocation vertexMetadata;
    OffsetAllocator::Allocation indexMetadata;
    OffsetAllocator::Allocation meshDecompressionMetadata;
    OffsetAllocator::Allocation skinningMetadata;
    OffsetAllocator::Allocation morphDeltaMetadata;
    OffsetAllocator::Allocation morphVertexBaseMetadata;
    OffsetAllocator::Allocation morphVertexCountMetadata;
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
    uint32_t bindlessIndex{0}; // GPU bindless texture index
    HotData() = default;

    explicit HotData(uint32_t index) : bindlessIndex(index)
    {
    }
  };

  struct ColdData
  {
    vk::TextureDesc textureDesc;
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
    uint32_t bindlessIndex = 0;
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
    uint32_t materialOffset;
    RenderQueue renderQueue = RenderQueue::Opaque;

    HotData() = default;

    explicit HotData(uint32_t offset, RenderQueue queue = RenderQueue::Opaque) : materialOffset(offset),
                                                                                 renderQueue(queue)
    {
    }
  };

  struct ColdData
  {
    Material material;
    OffsetAllocator::Allocation metadata;
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
