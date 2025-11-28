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

// ============================================================================
// Compressed Vertex Format (16 bytes vs 48 bytes uncompressed)
// ============================================================================

struct CompressedVertex
{
  int16_t pos_x, pos_y, pos_z;  // SNORM16: position in [-1,1] mesh-local space
  int16_t normal_x;              // SNORM16: normal.x
  uint32_t packed;               // RGB10A2 SNORM: [normal.y|tangent.xy|nz_sign|handedness]
  uint16_t uv_x, uv_y;          // UNORM16: UV in [0,1] mesh-local space
};
static_assert(sizeof(CompressedVertex) == 16, "CompressedVertex must be 16 bytes");

struct SkinnedVertex
{
  glm::vec4 position;       // xyz = position, w unused (keeps std430 stride aligned)
  uint32_t uvPacked;        // UNORM16 pair (packUnorm2x16)
  uint32_t normalPacked;    // packSnorm3x10_1x2(normal, 0.0)
  uint32_t tangentPacked;   // packSnorm3x10_1x2(tangent.xyz, handedness)
  uint32_t padding = 0;     // Explicit padding to keep struct 32 bytes on CPU/GPU
};
static_assert(sizeof(SkinnedVertex) == 32, "SkinnedVertex must be 32 bytes");
static_assert(offsetof(SkinnedVertex, uvPacked) == 16, "SkinnedVertex UV offset mismatch");
static_assert(offsetof(SkinnedVertex, normalPacked) == 20, "SkinnedVertex normal offset mismatch");
static_assert(offsetof(SkinnedVertex, tangentPacked) == 24, "SkinnedVertex tangent offset mismatch");
static_assert(offsetof(SkinnedVertex, padding) == 28, "SkinnedVertex padding offset mismatch");

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

constexpr uint32_t MORPH_COUNT_MASK = 0xFFFFu;
constexpr uint32_t MORPH_DELTA_SHIFT = 16u;

inline constexpr uint32_t packMorphCounts(uint32_t morphTargetCount, uint32_t morphDeltaCount)
{
  return ((morphDeltaCount & MORPH_COUNT_MASK) << MORPH_DELTA_SHIFT) |
         (morphTargetCount & MORPH_COUNT_MASK);
}

inline constexpr uint32_t unpackMorphTargetCount(uint32_t packedCounts)
{
  return packedCounts & MORPH_COUNT_MASK;
}

inline constexpr uint32_t unpackMorphDeltaCount(uint32_t packedCounts)
{
  return (packedCounts >> MORPH_DELTA_SHIFT) & MORPH_COUNT_MASK;
}

struct GPUAnimatedInstance
{
  glm::uvec4 baseInfo;     // srcBaseVertex, dstBaseVertex, vertexCount, meshDecompIndex
  glm::uvec4 skinningInfo; // skinningOffset, boneMatrixOffset, jointCount, morphStateOffset
  glm::uvec4 morphInfo;    // morphDeltaOffset, morphVertexBaseOffset, morphVertexCountOffset, packed morph counts
};
static_assert(sizeof(GPUAnimatedInstance) == sizeof(glm::uvec4) * 3, "GPUAnimatedInstance must be 48 bytes for std430 compatibility");

// Mesh-specific decompression parameters (static, uploaded once per mesh)
struct MeshDecompressionData
{
    glm::vec3 bbox_center;      // Offset 0,  Size 12
    float     _pad0;            // Offset 12, Size 4  (Aligns next vec3 to 16)

    glm::vec3 bbox_half_extent; // Offset 16, Size 12
    float     _pad1;            // Offset 28, Size 4  (Aligns next vec2 to 32)

    glm::vec2 uv_min;           // Offset 32, Size 8
    glm::vec2 uv_scale;         // Offset 40, Size 8
    // Explicit padding to reach 64 bytes total stride
    uint32_t  padding[4];       // Offset 48, Size 16 (Covers offsets 48, 52, 56, 60)
};
// Update the static assert to reflect the new, correct size
static_assert(sizeof(MeshDecompressionData) == 64, "MeshDecompressionData must be 64 bytes for std430 compatibility");

// Frame constants uploaded once per frame (replaces frame data in push constants)
struct FrameConstantsGPU
{
  mat4 viewProj;
  vec4 cameraPos;
  vec2 screenDims;
  float zNear;
  float zFar;
};
static_assert(sizeof(FrameConstantsGPU) == 96, "FrameConstantsGPU must be 96 bytes");

// Visibility buffer encoding: [drawID:20 bits][primitiveID:12 bits]
namespace VisibilityBuffer
{
    constexpr uint32_t DRAW_ID_BITS = 20;
    constexpr uint32_t PRIM_ID_BITS = 12;
    constexpr uint32_t DRAW_ID_MASK = (1u << DRAW_ID_BITS) - 1;
    constexpr uint32_t PRIM_ID_MASK = (1u << PRIM_ID_BITS) - 1;

    inline uint32_t Pack(uint32_t drawId, uint32_t primId)
    {
        return (drawId & DRAW_ID_MASK) | ((primId & PRIM_ID_MASK) << DRAW_ID_BITS);
    }

    inline void Unpack(uint32_t packed, uint32_t& drawId, uint32_t& primId)
    {
        drawId = packed & DRAW_ID_MASK;
        primId = (packed >> DRAW_ID_BITS) & PRIM_ID_MASK;
    }
}

// ============================================================================
// Compression Utilities
// ============================================================================

namespace VertexCompression {

    constexpr float VERTEX_EPSILON = 0.001f;

    inline int16_t encodeSnorm16(float value)
    {
        value = glm::clamp(value, -1.0f, 1.0f);
        return static_cast<int16_t>(value * 32767.0f);
    }

    inline uint16_t encodeUnorm16(float value)
    {
        value = glm::clamp(value, 0.0f, 1.0f);
        return static_cast<uint16_t>(value * 65535.0f);
    }

    inline uint32_t packNormalTangentSigns(float ny, float tx, float ty, float nx_sign, float handedness)
    {
        // Remap from [-1,1] to [0,1] for UNORM storage
        float ny_unorm = ny * 0.5f + 0.5f;
        float tx_unorm = tx * 0.5f + 0.5f;
        float ty_unorm = ty * 0.5f + 0.5f;

        // Pack as UNORM10 values
        uint32_t ny_bits = static_cast<uint32_t>(glm::clamp(ny_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;
        uint32_t tx_bits = static_cast<uint32_t>(glm::clamp(tx_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;
        uint32_t ty_bits = static_cast<uint32_t>(glm::clamp(ty_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;

        // Alpha encodes nx_sign (bit 1) and handedness (bit 0)
        uint32_t nx_sign_bit = (nx_sign < 0.0f) ? 1u : 0u;
        uint32_t handedness_bit = (handedness < 0.0f) ? 1u : 0u;
        uint32_t alpha_bits = (nx_sign_bit << 1) | handedness_bit;

        uint32_t packed = ny_bits | (tx_bits << 10) | (ty_bits << 20) | (alpha_bits << 30);
        return packed;
    }

    inline MeshDecompressionData computeDecompressionData(const Vertex* vertices, size_t count)
    {
        if (count == 0) {
            return MeshDecompressionData{
              .bbox_center = vec3(0.0f), ._pad0 = 0.0f,
              .bbox_half_extent = vec3(1.0f), ._pad1 = 0.0f,
              .uv_min = vec2(0.0f), .uv_scale = vec2(1.0f),
              .padding = {0, 0, 0, 0}
            };
        }

        vec3 bbox_min = vertices[0].position;
        vec3 bbox_max = vertices[0].position;
        vec2 uv_min = vertices[0].getUV();
        vec2 uv_max = vertices[0].getUV();

        for (size_t i = 1; i < count; ++i) {
            bbox_min = glm::min(bbox_min, vertices[i].position);
            bbox_max = glm::max(bbox_max, vertices[i].position);
            vec2 uv = vertices[i].getUV();
            uv_min = glm::min(uv_min, uv);
            uv_max = glm::max(uv_max, uv);
        }

        vec3 bbox_center = (bbox_min + bbox_max) * 0.5f;
        vec3 bbox_half_extent = (bbox_max - bbox_min) * 0.5f;
        bbox_half_extent = glm::max(bbox_half_extent, vec3(0.0001f));

        vec2 uv_scale = uv_max - uv_min;
        if (uv_scale.x < 0.0001f) uv_scale.x = 1.0f;
        if (uv_scale.y < 0.0001f) uv_scale.y = 1.0f;

        return MeshDecompressionData{
          .bbox_center = bbox_center, ._pad0 = 0.0f,
          .bbox_half_extent = bbox_half_extent, ._pad1 = 0.0f,
          .uv_min = uv_min, .uv_scale = uv_scale,
          .padding = {0, 0, 0, 0}
        };
    }

    inline CompressedVertex compress(const Vertex& v, const MeshDecompressionData& decomp)
    {
        CompressedVertex cv;

        // Position
        vec3 pos_local = (v.position - decomp.bbox_center) / decomp.bbox_half_extent;
        cv.pos_x = encodeSnorm16(pos_local.x);
        cv.pos_y = encodeSnorm16(pos_local.y);
        cv.pos_z = encodeSnorm16(pos_local.z);

        // Normal
        vec3 n = glm::normalize(v.normal);
        
        // Store |nx| in normal_x, with nz_sign encoded in its sign bit
        float nx_magnitude = glm::abs(n.x);
        float nx_sign = (n.x < 0.0f) ? -1.0f : 1.0f;
        float nz_sign = (n.z < 0.0f) ? -1.0f : 1.0f;
        
        // Encode: magnitude with nz_sign in the sign bit
        float nx_with_nz_sign = nx_magnitude * nz_sign;
        cv.normal_x = encodeSnorm16(nx_with_nz_sign);

        // Tangent: flip entire tangent if tz < 0 to force tz positive
        vec3 t = glm::normalize(vec3(v.tangent));
        float handedness = v.tangent.w;

        if (t.z < 0.0f) {
            t = -t;  // Flip entire tangent vector
            handedness = -handedness;  // Also flip handedness to maintain TBN consistency
        }

        // Pack ny, tx, ty as UNORM, with nx_sign and handedness in alpha bits
        cv.packed = packNormalTangentSigns(n.y, t.x, t.y, nx_sign, handedness);

        // UV
        vec2 uv = v.getUV();
        vec2 uv_local = (uv - decomp.uv_min) / decomp.uv_scale;
        cv.uv_x = encodeUnorm16(uv_local.x);
        cv.uv_y = encodeUnorm16(uv_local.y);

        return cv;
    }

} // namespace VertexCompression

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

namespace VertexCompression {

// Decode SNORM16 to float (mirrors hardware conversion)
inline float decodeSnorm16(int16_t value)
{
    return glm::clamp(static_cast<float>(value) / 32767.0f, -1.0f, 1.0f);
}

// Decode UNORM16 to float (mirrors hardware conversion)
inline float decodeUnorm16(uint16_t value)
{
    return static_cast<float>(value) / 65535.0f;
}

// Validate compression - mirrors shader decompression exactly
inline bool validateCompression(
    const std::vector<Vertex>& original,
    const std::vector<CompressedVertex>& compressed,
    const MeshDecompressionData& decomp,
    float pos_epsilon = 0.01f,
    float uv_epsilon = 0.001f,
    float normal_dot_threshold = 0.999f,
    float tangent_dot_threshold = 0.999f)
{
    if (original.size() != compressed.size()) {
        printf("[ERROR] Size mismatch: original=%zu, compressed=%zu\n",
            original.size(), compressed.size());
        return false;
    }

    bool all_valid = true;
    size_t error_count = 0;
    const size_t max_errors_to_print = 10;

    for (size_t i = 0; i < original.size(); ++i) {
        const Vertex& orig = original[i];
        const CompressedVertex& comp = compressed[i];
        vec3 pos_snorm(decodeSnorm16(comp.pos_x), decodeSnorm16(comp.pos_y), decodeSnorm16(comp.pos_z));
        vec3 decompressed_pos = pos_snorm * decomp.bbox_half_extent + decomp.bbox_center;

        float pos_error = glm::distance(orig.position, decompressed_pos);
        if (pos_error > pos_epsilon) {
            if (error_count++ < max_errors_to_print) {
                printf("[V%zu] POSITION ERROR: %.6f\n", i, pos_error);
                printf("  Original:     (%.6f, %.6f, %.6f)\n",
                    orig.position.x, orig.position.y, orig.position.z);
                printf("  Decompressed: (%.6f, %.6f, %.6f)\n",
                    decompressed_pos.x, decompressed_pos.y, decompressed_pos.z);
            }
            all_valid = false;
        }

        float nx_encoded = decodeSnorm16(comp.normal_x);

        // Decode UNORM10 values and remap from [0,1] to [-1,1]
        float ny_unorm = static_cast<float>(comp.packed & 0x3FFu) / 1023.0f;
        float ny = ny_unorm * 2.0f - 1.0f;

        uint32_t alpha_bits = (comp.packed >> 30) & 0x3u;
        
        // Extract nx_sign from alpha bit 1
        bool nx_negative = (alpha_bits & 2u) != 0u;
        float nx_sign = nx_negative ? -1.0f : 1.0f;
        
        // Extract nz_sign from the sign of the encoded normal_x
        float nz_sign = (nx_encoded < 0.0f) ? -1.0f : 1.0f;
        
        // Reconstruct nx: magnitude from |nx_encoded|, sign from nx_sign
        float nx_magnitude = glm::abs(nx_encoded);
        float nx = nx_magnitude * nx_sign;
        
        float nz = glm::sqrt(glm::max(VERTEX_EPSILON, 1.0f - nx * nx - ny * ny)) * nz_sign;

        vec3 decompressed_normal = glm::normalize(vec3(nx, ny, nz));
        vec3 original_normal = glm::normalize(orig.normal);

        float normal_dot = glm::dot(original_normal, decompressed_normal);
        if (normal_dot < normal_dot_threshold) {
            if (error_count++ < max_errors_to_print) {
                printf("[V%zu] NORMAL ERROR: dot=%.6f (angle=%.2f deg)\n",
                    i, normal_dot, glm::degrees(glm::acos(normal_dot)));
                printf("  Original:     (%.6f, %.6f, %.6f)\n",
                    original_normal.x, original_normal.y, original_normal.z);
                printf("  Decompressed: (%.6f, %.6f, %.6f)\n",
                    decompressed_normal.x, decompressed_normal.y, decompressed_normal.z);
            }
            all_valid = false;
        }

        // Decode UNORM10 values and remap from [0,1] to [-1,1]
        float tx_unorm = static_cast<float>((comp.packed >> 10) & 0x3FFu) / 1023.0f;
        float ty_unorm = static_cast<float>((comp.packed >> 20) & 0x3FFu) / 1023.0f;
        float tx = tx_unorm * 2.0f - 1.0f;
        float ty = ty_unorm * 2.0f - 1.0f;
        float tz = glm::sqrt(glm::max(VERTEX_EPSILON, 1.0f - tx * tx - ty * ty));

        vec3 decompressed_tangent = glm::normalize(vec3(tx, ty, tz));

        vec3 original_tangent = glm::normalize(vec3(orig.tangent));
        float original_handedness = orig.tangent.w;
        if (original_tangent.z < 0.0f) {
            original_tangent = -original_tangent;
            original_handedness = -original_handedness;
        }

        float tangent_dot = glm::dot(original_tangent, decompressed_tangent);
        if (tangent_dot < tangent_dot_threshold) {
            if (error_count++ < max_errors_to_print) {
                printf("[V%zu] TANGENT ERROR: dot=%.6f (angle=%.2f deg)\n",
                    i, tangent_dot, glm::degrees(glm::acos(glm::abs(tangent_dot))));
                printf("  Original:     (%.6f, %.6f, %.6f)\n",
                    original_tangent.x, original_tangent.y, original_tangent.z);
                printf("  Decompressed: (%.6f, %.6f, %.6f)\n",
                    decompressed_tangent.x, decompressed_tangent.y, decompressed_tangent.z);
            }
            all_valid = false;
        }

        vec2 uv_unorm(decodeUnorm16(comp.uv_x), decodeUnorm16(comp.uv_y));
        vec2 decompressed_uv = uv_unorm * decomp.uv_scale + decomp.uv_min;

        float uv_error = glm::distance(orig.getUV(), decompressed_uv);
        if (uv_error > uv_epsilon) {
            if (error_count++ < max_errors_to_print) {
                printf("[V%zu] UV ERROR: %.6f\n", i, uv_error);
                printf("  Original:     (%.6f, %.6f)\n", orig.uv_x, orig.uv_y);
                printf("  Decompressed: (%.6f, %.6f)\n", decompressed_uv.x, decompressed_uv.y);
            }
            all_valid = false;
        }

        bool handedness_negative = (alpha_bits & 1u) != 0u;
        float handedness = handedness_negative ? -1.0f : 1.0f;

        // Compare handedness
        if (glm::sign(original_handedness) != glm::sign(handedness)) {
            printf("[V%zu] HANDEDNESS MISMATCH: orig=%.1f, decompressed=%.1f\n",
                i, original_handedness, handedness);
            all_valid = false;
        }

        // Check bitangent with correct handedness
        vec3 computed_bitangent = glm::cross(decompressed_normal, decompressed_tangent) * handedness;
        vec3 expected_bitangent = glm::cross(original_normal, original_tangent) * original_handedness;
        float bitangent_dot = glm::dot(glm::normalize(computed_bitangent),
            glm::normalize(expected_bitangent));

        if (bitangent_dot < 0.0f) {
            if (error_count++ < max_errors_to_print) {
                printf("[V%zu] HANDEDNESS LOST: tangent.w=%.1f (bitangent flipped in shader)\n",
                    i, original_handedness);
            }
        }
    }

    if (error_count > max_errors_to_print) {
        printf("... and %zu more errors (showing first %zu)\n",
            error_count - max_errors_to_print, max_errors_to_print);
    }

    if (all_valid) {
        printf("[SUCCESS] All %zu vertices validated successfully\n", original.size());
    }
    else {
        printf("[FAILED] %zu vertices had errors\n", error_count);
    }

    return all_valid;
}

} // namespace VertexCompression
