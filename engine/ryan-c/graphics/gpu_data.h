#pragma once
#include "math/utils_math.h"

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

struct MaterialData
{
  vec4 baseColorFactor;
  vec4 metallicRoughnessNormalOcclusion; // packed: metallic, roughness, normal, occlusion
  vec4 emissiveFactorAlphaCutoff;        // emissive.rgb, alphaCutoff

  uint32_t baseColorTexture;
  uint32_t metallicRoughnessTexture;
  uint32_t normalTexture;
  uint32_t emissiveTexture;
  uint32_t occlusionTexture;

  uint32_t materialTypeFlags;
  uint32_t alphaMode;
  uint32_t flags;
  uint32_t padding;
};
