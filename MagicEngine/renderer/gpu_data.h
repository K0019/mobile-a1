#pragma once
#include "math/utils_math.h"
#include <glm/gtc/packing.hpp>

struct Vertex
{
  vec3 position;
  float uv_x;
  vec3 normal;
  float uv_y;
  vec4 tangent; // xyz = tangent direction, w = bitangent handedness
  Vertex() = default;

  Vertex(const vec3& pos, const vec3& n, const vec2& uv) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(0.0f, 0.0f, 0.0f, 1.0f) {}

  Vertex(const vec3& pos, const vec3& n, const vec2& uv, const vec4& tan) : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(tan) {}

  // Helper to get UV as vec2
  vec2 getUV() const { return {uv_x, uv_y}; }

  void setUV(const vec2& uv)
  {
    uv_x = uv.x;
    uv_y = uv.y;
  }

  void setUV(float u, float v)
  {
    uv_x = u;
    uv_y = v;
  }
};

constexpr uint32_t GPU_INVALID_BONE_INDEX = 0xFFFFFFFFu;

struct GPUSkinningData
{
  glm::uvec4 indices{ GPU_INVALID_BONE_INDEX, GPU_INVALID_BONE_INDEX, GPU_INVALID_BONE_INDEX, GPU_INVALID_BONE_INDEX };
  glm::vec4 weights{ 0.0f };
};
static_assert(sizeof(GPUSkinningData) == 32, "GPUSkinningData must be 32 bytes for std430 compatibility");

struct GPUMorphDelta
{
  uint32_t morphTargetIndex = 0;
  uint32_t vertexIndex = 0;
  uint32_t _pad0 = 0;
  uint32_t _pad1 = 0;
  glm::vec4 deltaPosition{ 0.0f };
  glm::vec4 deltaNormal{ 0.0f };
  glm::vec4 deltaTangent{ 0.0f };
};
static_assert(sizeof(GPUMorphDelta) == 64, "GPUMorphDelta must be 64 bytes for std430 compatibility");

struct MaterialData
{
  vec4 baseColorFactor;
  vec4 metallicRoughnessNormalOcclusion; // packed: metallic, roughness, normal, occlusion
  vec4 emissiveFactorAlphaCutoff;        // emissive.rgb, alphaCutoff

  uint32_t baseColorTexture;
  uint32_t normalTexture;
  uint32_t metallicRoughnessTexture;
  uint32_t emissiveTexture;
  uint32_t occlusionTexture;

  uint32_t materialTypeFlags;
  uint32_t flags;
  uint32_t padding[1];  
};

