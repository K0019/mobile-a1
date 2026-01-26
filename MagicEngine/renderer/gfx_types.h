/**
 * @file gfx_types.h
 * @brief Type aliases and packed data structures for graphics backend.
 *
 * This file provides:
 * - Type aliases mapping hina-vk types to the engine namespace
 * - Packed data structures for mobile-optimized rendering (HypeHype-style)
 * - Utility functions for vertex/material packing
 *
 * Part of the MagicEngine -> hina-vk migration (see docs/HINA_VK_MIGRATION_PLAN.md)
 */

#pragma once

#include <hina_vk.h>
#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp>
#include <cstdint>
#include <cmath>

// ============================================================================
// GfxRenderer Types - internal namespace to avoid conflicts with engine types
// ============================================================================
namespace gfx {

// Type aliases for hina-vk handles
using Buffer = hina_buffer;
using Texture = hina_texture;
using TextureView = hina_texture_view;
using Sampler = hina_sampler;
using Pipeline = hina_pipeline;
using BindGroup = hina_bind_group;
using BindGroupLayout = hina_bind_group_layout;
using QueryPool = hina_query_pool;
using Shader = hina_shader;
using Cmd = hina_cmd;
using Context = hina_context;
using SwapchainImage = hina_swapchain_image;
using SyncPoint = hina_sync_point;
using Ticket = hina_ticket;

// Queue types
using Queue = hina_queue;
constexpr Queue QUEUE_GRAPHICS = HINA_QUEUE_GRAPHICS;
constexpr Queue QUEUE_COMPUTE = HINA_QUEUE_COMPUTE;
constexpr Queue QUEUE_TRANSFER = HINA_QUEUE_TRANSFER;

// ============================================================================
// Generational Handles (HypeHype-style)
// ============================================================================

/**
 * @brief Lightweight generational handle for resource pooling.
 * 16-bit index + 16-bit generation = 32-bit total.
 * Catches use-after-free bugs via generation mismatch.
 */
template<typename T>
struct Handle {
    uint16_t index = 0;
    uint16_t generation = 0;

    [[nodiscard]] bool isValid() const { return generation != 0; }
    [[nodiscard]] bool empty() const { return generation == 0; }
    explicit operator bool() const { return generation != 0; }

    bool operator==(const Handle& other) const {
        return index == other.index && generation == other.generation;
    }
    bool operator!=(const Handle& other) const {
        return !(*this == other);
    }
};

// Typed handles for common resources
using PipelineHandle = Handle<struct PipelineTag>;
using MaterialHandle = Handle<struct MaterialTag>;
using MeshHandle = Handle<struct MeshTag>;
using TextureHandle = Handle<struct TextureTag>;

// ============================================================================
// Octahedral Encoding (for normals/tangents/shadow maps)
// ============================================================================

namespace oct {

/**
 * @brief Encode unit vector to octahedral UV [0,1]^2
 * From "A Survey of Efficient Representations for Independent Unit Vectors" (JCGT 2014)
 */
inline glm::vec2 encode(glm::vec3 n) {
    // Project to octahedron
    n /= (std::abs(n.x) + std::abs(n.y) + std::abs(n.z));

    // Fold lower hemisphere
    if (n.z < 0.0f) {
        float nx = n.x;
        float ny = n.y;
        n.x = (1.0f - std::abs(ny)) * (nx >= 0.0f ? 1.0f : -1.0f);
        n.y = (1.0f - std::abs(nx)) * (ny >= 0.0f ? 1.0f : -1.0f);
    }

    // Map to [0,1]
    return glm::vec2(n.x * 0.5f + 0.5f, n.y * 0.5f + 0.5f);
}

/**
 * @brief Decode octahedral UV [0,1]^2 to unit vector
 */
inline glm::vec3 decode(glm::vec2 f) {
    f = f * 2.0f - 1.0f;
    glm::vec3 n = glm::vec3(f.x, f.y, 1.0f - std::abs(f.x) - std::abs(f.y));

    float t = glm::max(-n.z, 0.0f);
    n.x += (n.x >= 0.0f ? -t : t);
    n.y += (n.y >= 0.0f ? -t : t);

    return glm::normalize(n);
}

/**
 * @brief Encode to RGB10A2 format (20 bits for octahedral + 2 bits extra)
 * @param n Unit normal vector
 * @param extra 2-bit value to store in alpha (e.g., material ID, sign bit)
 */
inline uint32_t encodeRGB10A2(glm::vec3 n, uint32_t extra = 0) {
    glm::vec2 oct = encode(n);
    uint32_t x = static_cast<uint32_t>(glm::clamp(oct.x, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FF;
    uint32_t y = static_cast<uint32_t>(glm::clamp(oct.y, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FF;
    return x | (y << 10) | ((extra & 0x3) << 30);
}

/**
 * @brief Decode from RGB10A2 format
 * @param packed RGB10A2 packed value
 * @param extra Output: 2-bit alpha value
 */
inline glm::vec3 decodeRGB10A2(uint32_t packed, uint32_t* extra = nullptr) {
    float x = static_cast<float>(packed & 0x3FF) / 1023.0f;
    float y = static_cast<float>((packed >> 10) & 0x3FF) / 1023.0f;
    if (extra) *extra = (packed >> 30) & 0x3;
    return decode(glm::vec2(x, y));
}

} // namespace oct

// ============================================================================
// Packed Vertex Format (24 bytes, HypeHype-style)
// ============================================================================

/**
 * @brief Position stream (12 bytes) - used alone for shadows, TBDR binning
 */
struct VertexPosition {
    float x, y, z;

    VertexPosition() = default;
    VertexPosition(float px, float py, float pz) : x(px), y(py), z(pz) {}
    explicit VertexPosition(const glm::vec3& p) : x(p.x), y(p.y), z(p.z) {}

    glm::vec3 toVec3() const { return glm::vec3(x, y, z); }
};
static_assert(sizeof(VertexPosition) == 12, "VertexPosition must be 12 bytes");

/**
 * @brief Attribute stream (12 bytes) - normals, tangents, UVs
 *
 * Layout matches main branch's CompressedVertex encoding:
 *   normalX:  lower 16 bits = SNORM16 |nx| with nz_sign in sign bit
 *   packed:   RGB10A2 - ny(10), tx(10), ty(10), alpha(2) where alpha = (nx_sign<<1)|handedness
 *   uv:       RG16_UNORM - UV scaled to [0,1] from mesh-local range
 */
struct VertexAttributes {
    uint32_t normalX;   // lower 16 bits: SNORM16 |nx| * nz_sign
    uint32_t packed;    // RGB10A2: ny, tx, ty, (nx_sign<<1)|handedness
    uint32_t uv;        // RG16_UNORM: UV

    VertexAttributes() = default;

    /**
     * @brief Pack vertex attributes using main branch's CompressedVertex encoding
     */
    void pack(const glm::vec3& normal, const glm::vec3& tangent,
              float handedness, const glm::vec2& texcoord, uint32_t /*localMaterialID*/ = 0) {
        glm::vec3 n = glm::normalize(normal);
        glm::vec3 t = glm::normalize(tangent);

        // Store |nx| with nz_sign in sign bit (matches main branch)
        float nx_magnitude = std::abs(n.x);
        float nx_sign = (n.x < 0.0f) ? -1.0f : 1.0f;
        float nz_sign = (n.z < 0.0f) ? -1.0f : 1.0f;
        float nx_with_nz_sign = nx_magnitude * nz_sign;

        // Encode as SNORM16 in lower 16 bits
        int16_t normal_x_snorm = static_cast<int16_t>(glm::clamp(nx_with_nz_sign, -1.0f, 1.0f) * 32767.0f);
        normalX = static_cast<uint32_t>(static_cast<uint16_t>(normal_x_snorm));

        // Flip tangent if tz < 0 to force tz positive (matches main branch)
        float h = handedness;
        if (t.z < 0.0f) {
            t = -t;
            h = -h;
        }

        // Pack ny, tx, ty as UNORM10 (remap from [-1,1] to [0,1])
        float ny_unorm = n.y * 0.5f + 0.5f;
        float tx_unorm = t.x * 0.5f + 0.5f;
        float ty_unorm = t.y * 0.5f + 0.5f;

        uint32_t ny_bits = static_cast<uint32_t>(glm::clamp(ny_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;
        uint32_t tx_bits = static_cast<uint32_t>(glm::clamp(tx_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;
        uint32_t ty_bits = static_cast<uint32_t>(glm::clamp(ty_unorm, 0.0f, 1.0f) * 1023.0f + 0.5f) & 0x3FFu;

        // Alpha: bit 1 = nx_sign negative, bit 0 = handedness negative
        uint32_t nx_sign_bit = (nx_sign < 0.0f) ? 1u : 0u;
        uint32_t handedness_bit = (h < 0.0f) ? 1u : 0u;
        uint32_t alpha_bits = (nx_sign_bit << 1) | handedness_bit;

        packed = ny_bits | (tx_bits << 10) | (ty_bits << 20) | (alpha_bits << 30);
        uv = glm::packUnorm2x16(texcoord);
    }

    /**
     * @brief Unpack vertex attributes
     */
    void unpack(glm::vec3& normal, glm::vec3& tangent,
                float& handedness, glm::vec2& texcoord, uint32_t& /*localMaterialID*/) const {
        // Decode normal_x from SNORM16
        int16_t normal_x_snorm = static_cast<int16_t>(normalX & 0xFFFFu);
        float normal_x_float = glm::clamp(static_cast<float>(normal_x_snorm) / 32767.0f, -1.0f, 1.0f);

        float nx_magnitude = std::abs(normal_x_float);
        float nz_sign = (normal_x_float < 0.0f) ? -1.0f : 1.0f;

        // Decode packed RGB10A2
        float ny = (static_cast<float>(packed & 0x3FFu) / 1023.0f) * 2.0f - 1.0f;
        float tx = (static_cast<float>((packed >> 10) & 0x3FFu) / 1023.0f) * 2.0f - 1.0f;
        float ty = (static_cast<float>((packed >> 20) & 0x3FFu) / 1023.0f) * 2.0f - 1.0f;
        uint32_t alpha = (packed >> 30) & 0x3u;

        float nx_sign = ((alpha >> 1) & 1u) ? -1.0f : 1.0f;
        handedness = (alpha & 1u) ? -1.0f : 1.0f;

        float nx = nx_magnitude * nx_sign;
        float nz = std::sqrt(std::max(0.001f, 1.0f - nx * nx - ny * ny)) * nz_sign;
        normal = glm::normalize(glm::vec3(nx, ny, nz));

        float tz = std::sqrt(std::max(0.001f, 1.0f - tx * tx - ty * ty));
        tangent = glm::normalize(glm::vec3(tx, ty, tz));

        texcoord = glm::unpackUnorm2x16(uv);
    }
};
static_assert(sizeof(VertexAttributes) == 12, "VertexAttributes must be 12 bytes");

// ============================================================================
// Skinning Vertex Stream (8 bytes) - for skeletal animation
// ============================================================================

/**
 * @brief Skinning stream (8 bytes) - bone indices and weights.
 *
 * Mobile-optimized: Uses R8G8B8A8_UINT for indices (max 255 bones per mesh)
 * and R8G8B8A8_UNORM for weights (8-bit precision is sufficient for blending).
 *
 * This is 75% smaller than GPUSkinningData (32 bytes) for bandwidth reduction.
 */
struct VertexSkinning {
    uint8_t boneIndices[4];  // 4 bytes - bone indices 0-255 (we limit to 64)
    uint8_t weights[4];      // 4 bytes - weights as UNORM8 (0-255 maps to 0.0-1.0)

    VertexSkinning() = default;

    /**
     * @brief Pack from full-precision skinning data to compact format.
     * @param indices Bone indices (values clamped to 0-255)
     * @param weightsIn Blend weights (values clamped to 0.0-1.0)
     */
    void pack(const uint32_t* indices, const float* weightsIn) {
        for (int i = 0; i < 4; ++i) {
            // Clamp index to 255 (though we limit to 64 bones)
            boneIndices[i] = static_cast<uint8_t>(std::min(indices[i], 255u));
            // Quantize weight to 8-bit UNORM
            weights[i] = static_cast<uint8_t>(glm::clamp(weightsIn[i], 0.0f, 1.0f) * 255.0f + 0.5f);
        }
    }

    /**
     * @brief Unpack to full-precision for debugging/CPU use.
     */
    void unpack(uint32_t* indices, float* weightsOut) const {
        for (int i = 0; i < 4; ++i) {
            indices[i] = boneIndices[i];
            weightsOut[i] = static_cast<float>(weights[i]) / 255.0f;
        }
    }
};
static_assert(sizeof(VertexSkinning) == 8, "VertexSkinning must be 8 bytes");

// ============================================================================
// Packed Material Data (16 bytes = 8x fp16)
// ============================================================================

/**
 * @brief Packed material data using fp16 for mobile efficiency.
 *
 * Single indexed load on GPU (16 bytes = 1x uint4).
 * Layout:
 *   [0]: baseColor.r, baseColor.g  (half2)
 *   [1]: baseColor.b, baseColor.a  (half2) - alpha for transparency
 *   [2]: roughness, metallic       (half2)
 *   [3]: emissive, alphaCutoff     (half2) - alphaCutoff for alpha testing
 */
struct PackedMaterial {
    uint32_t data[4];

    PackedMaterial() { data[0] = data[1] = data[2] = data[3] = 0; }

    void pack(const glm::vec4& baseColor, float roughness, float metallic, float emissive, float alphaCutoff) {
        // Clamp roughness to minimum 0.089 for fp16 precision (Filament recommendation)
        roughness = glm::max(roughness, 0.089f);

        data[0] = glm::packHalf2x16(glm::vec2(baseColor.r, baseColor.g));
        data[1] = glm::packHalf2x16(glm::vec2(baseColor.b, baseColor.a));
        data[2] = glm::packHalf2x16(glm::vec2(roughness, metallic));
        data[3] = glm::packHalf2x16(glm::vec2(emissive, alphaCutoff));
    }

    void unpack(glm::vec4& baseColor, float& roughness, float& metallic, float& emissive, float& alphaCutoff) const {
        glm::vec2 rg = glm::unpackHalf2x16(data[0]);
        glm::vec2 ba = glm::unpackHalf2x16(data[1]);
        glm::vec2 rm = glm::unpackHalf2x16(data[2]);
        glm::vec2 ec = glm::unpackHalf2x16(data[3]);

        baseColor = glm::vec4(rg.x, rg.y, ba.x, ba.y);
        roughness = rm.x;
        metallic = rm.y;
        emissive = ec.x;
        alphaCutoff = ec.y;
    }
};
static_assert(sizeof(PackedMaterial) == 16, "PackedMaterial must be 16 bytes");

// ============================================================================
// Packed Instance Data (96 bytes = 6x float4)
// ============================================================================

/**
 * @brief Per-instance transform and metadata for GPU instancing.
 *
 * Layout (std140 compatible):
 *   float4[0-2]: 4x3 affine transform matrix (row-major)
 *   float4[3-5]: 3x3 normal matrix + packed material IDs
 */
struct PackedInstance {
    // 4x3 affine matrix (48 bytes)
    float m00, m01, m02, m03;  // row 0 + translation.x
    float m10, m11, m12, m13;  // row 1 + translation.y
    float m20, m21, m22, m23;  // row 2 + translation.z

    // 3x3 normal matrix + packed data (48 bytes)
    float n00, n01, n02;
    uint32_t packedMaterialIDs;  // 4x 8-bit material indices
    float n10, n11, n12;
    uint32_t flags;              // instance flags
    float n20, n21, n22;
    uint32_t objectID;           // for picking/visibility buffer

    PackedInstance() = default;

    void setTransform(const glm::mat4& model) {
        // Extract 3x3 rotation/scale
        m00 = model[0][0]; m01 = model[1][0]; m02 = model[2][0]; m03 = model[3][0];
        m10 = model[0][1]; m11 = model[1][1]; m12 = model[2][1]; m13 = model[3][1];
        m20 = model[0][2]; m21 = model[1][2]; m22 = model[2][2]; m23 = model[3][2];

        // Compute normal matrix (inverse transpose of 3x3)
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));
        n00 = normalMat[0][0]; n01 = normalMat[1][0]; n02 = normalMat[2][0];
        n10 = normalMat[0][1]; n11 = normalMat[1][1]; n12 = normalMat[2][1];
        n20 = normalMat[0][2]; n21 = normalMat[1][2]; n22 = normalMat[2][2];
    }

    void setMaterialIDs(uint8_t mat0, uint8_t mat1 = 0, uint8_t mat2 = 0, uint8_t mat3 = 0) {
        packedMaterialIDs = mat0 | (mat1 << 8) | (mat2 << 16) | (mat3 << 24);
    }

    uint8_t getMaterialID(uint32_t localIdx) const {
        return static_cast<uint8_t>((packedMaterialIDs >> (localIdx * 8)) & 0xFF);
    }
};
static_assert(sizeof(PackedInstance) == 96, "PackedInstance must be 96 bytes");

// ============================================================================
// Aligned Transform for Dynamic UBO Offsets (256 bytes)
// ============================================================================

/**
 * @brief Transform data aligned to 256 bytes for dynamic UBO offsets.
 * minUniformBufferOffsetAlignment is typically 256 bytes on mobile GPUs.
 * Contains mat4 model (64 bytes) with padding to reach 256-byte alignment.
 */
struct alignas(256) AlignedTransform {
    glm::mat4 model = glm::mat4(1.0f);   // 64 bytes
    uint32_t _pad[48] = {};               // 192 bytes padding to reach 256 total
};
static_assert(sizeof(AlignedTransform) == 256, "AlignedTransform must be 256 bytes");
static_assert(alignof(AlignedTransform) == 256, "AlignedTransform must have 256-byte alignment");

// ============================================================================
// Aligned Bone Matrices for Skinning (4KB, 256-byte aligned)
// ============================================================================

/**
 * @brief Per-instance bone matrix UBO for skinned meshes.
 *
 * 256 bones * mat4 (64 bytes each) = 16384 bytes (16 KB).
 * This fits within the minimum guaranteed UBO size (16 KB on mobile, 64 KB on desktop).
 * Aligned to 256 bytes for dynamic UBO offsets on mobile GPUs.
 */
constexpr uint32_t MAX_BONES_PER_MESH = 256;
constexpr uint32_t BONE_MATRICES_SIZE = MAX_BONES_PER_MESH * sizeof(glm::mat4);  // 16384 bytes

struct alignas(256) AlignedBoneMatrices {
    glm::mat4 bones[MAX_BONES_PER_MESH];  // 16384 bytes total (256-byte aligned)
};
static_assert(sizeof(AlignedBoneMatrices) == 16384, "AlignedBoneMatrices must be 16KB");
static_assert(alignof(AlignedBoneMatrices) == 256, "AlignedBoneMatrices must have 256-byte alignment");

// ============================================================================
// Frame Constants (bound once per frame in Set 0)
// ============================================================================

/**
 * @brief Per-frame constants for the global UBO.
 */
struct FrameConstants {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::mat4 invViewProj;
    glm::vec4 cameraPos;      // xyz = position, w = unused
    glm::vec4 cameraDir;      // xyz = forward direction, w = unused
    glm::vec2 screenSize;     // width, height in pixels
    glm::vec2 invScreenSize;  // 1/width, 1/height
    float zNear;
    float zFar;
    float time;               // total elapsed time
    float deltaTime;          // frame delta time
};
static_assert(sizeof(FrameConstants) == 320, "FrameConstants size check");

// ============================================================================
// Draw Stream (HypeHype-style state tracking)
// ============================================================================

/**
 * @brief Draw call state for batching.
 * Track dirty state to minimize bind changes.
 */
struct DrawState {
    uint16_t pso;              // Pipeline handle
    uint16_t materialBG;       // Material bind group handle
    uint16_t indexBuffer;      // Index buffer handle
    uint16_t vertexBuffer;     // Vertex buffer handle
    uint32_t transformOffset;  // Dynamic offset into transform UBO
    uint32_t indexCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;

    bool operator==(const DrawState& other) const {
        return pso == other.pso &&
               materialBG == other.materialBG &&
               indexBuffer == other.indexBuffer &&
               vertexBuffer == other.vertexBuffer;
    }
};

} // namespace gfx

// ============================================================================
// Lighting namespace - GPU light structures
// ============================================================================

namespace Lighting {

constexpr uint32_t MAX_SHADOW_POINT_LIGHTS = 4;

/**
 * @brief GPU-friendly light structure for the lighting UBO.
 * Matches the layout expected by deferred lighting shaders.
 */
struct GPULight {
    glm::vec3 position = glm::vec3(0.0f);
    float range = 10.0f;
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    float spotAngle = 0.0f;  // cos(outerConeAngle), 0 = point light
    uint32_t type = 1;        // 0 = directional, 1 = point, 2 = spot
    uint32_t padding[3] = {0, 0, 0};
};
static_assert(sizeof(GPULight) == 64, "GPULight must be 64 bytes for std140 alignment");

} // namespace Lighting
