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
 * Layout:
 *   normalMatID:  RGB10A2 - octahedral normal (20 bits) + 2-bit local material ID
 *   tangentSign:  RGB10A2 - octahedral tangent (20 bits) + bitangent sign (2 bits)
 *   uv:           RG16_UNORM - UV scaled to [0,1] from mesh-local range
 */
struct VertexAttributes {
    uint32_t normalMatID;   // RGB10A2: octahedral normal + 2-bit material
    uint32_t tangentSign;   // RGB10A2: octahedral tangent + bitangent sign
    uint32_t uv;            // RG16_UNORM: UV

    VertexAttributes() = default;

    /**
     * @brief Pack vertex attributes
     * @param normal Unit normal vector
     * @param tangent Unit tangent vector
     * @param bitangentSign Bitangent handedness (+1 or -1)
     * @param texcoord UV coordinates in [0,1]
     * @param localMaterialID 2-bit local material index (0-3)
     */
    void pack(const glm::vec3& normal, const glm::vec3& tangent,
              float bitangentSign, const glm::vec2& texcoord, uint32_t localMaterialID = 0) {
        normalMatID = oct::encodeRGB10A2(normal, localMaterialID & 0x3);
        tangentSign = oct::encodeRGB10A2(tangent, bitangentSign < 0.0f ? 1 : 0);
        uv = glm::packUnorm2x16(texcoord);
    }

    /**
     * @brief Unpack vertex attributes
     */
    void unpack(glm::vec3& normal, glm::vec3& tangent,
                float& bitangentSign, glm::vec2& texcoord, uint32_t& localMaterialID) const {
        normal = oct::decodeRGB10A2(normalMatID, &localMaterialID);
        uint32_t signBit;
        tangent = oct::decodeRGB10A2(tangentSign, &signBit);
        bitangentSign = (signBit & 1) ? -1.0f : 1.0f;
        texcoord = glm::unpackUnorm2x16(uv);
    }
};
static_assert(sizeof(VertexAttributes) == 12, "VertexAttributes must be 12 bytes");

// ============================================================================
// Packed Material Data (16 bytes = 8x fp16)
// ============================================================================

/**
 * @brief Packed material data using fp16 for mobile efficiency.
 *
 * Single indexed load on GPU (16 bytes = 1x uint4).
 * Layout:
 *   [0]: baseColor.r, baseColor.g  (half2)
 *   [1]: baseColor.b, roughness    (half2)
 *   [2]: metallic, emissive        (half2)
 *   [3]: rim, ao                   (half2)
 */
struct PackedMaterial {
    uint32_t data[4];

    PackedMaterial() { data[0] = data[1] = data[2] = data[3] = 0; }

    void pack(const glm::vec4& baseColor, float roughness, float metallic, float emissive, float ao, float rim = 0.0f) {
        // Clamp roughness to minimum 0.089 for fp16 precision (Filament recommendation)
        roughness = glm::max(roughness, 0.089f);

        data[0] = glm::packHalf2x16(glm::vec2(baseColor.r, baseColor.g));
        data[1] = glm::packHalf2x16(glm::vec2(baseColor.b, roughness));
        data[2] = glm::packHalf2x16(glm::vec2(metallic, emissive));
        data[3] = glm::packHalf2x16(glm::vec2(rim, ao));
    }

    void unpack(glm::vec4& baseColor, float& roughness, float& metallic, float& emissive, float& ao, float& rim) const {
        glm::vec2 rg = glm::unpackHalf2x16(data[0]);
        glm::vec2 br = glm::unpackHalf2x16(data[1]);
        glm::vec2 me = glm::unpackHalf2x16(data[2]);
        glm::vec2 ra = glm::unpackHalf2x16(data[3]);

        baseColor = glm::vec4(rg.x, rg.y, br.x, 1.0f);
        roughness = br.y;
        metallic = me.x;
        emissive = me.y;
        rim = ra.x;
        ao = ra.y;
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

/**
 * @brief Visible object after culling, ready for draw stream generation.
 */
struct VisibleObject {
    uint32_t psoHandle;
    uint32_t materialBGHandle;
    uint32_t meshHandle;
    uint32_t transformOffset;
    uint32_t indexCount;
    uint32_t firstIndex;
    int32_t  vertexOffset;
    float    sortKey;  // for front-to-back or back-to-front sorting
};

// ============================================================================
// Light Tile Binning (CPU-side, 32x32 tiles)
// ============================================================================

/**
 * @brief Per-tile light visibility mask.
 * 64 bits = max 64 lights per frame (HypeHype approach).
 */
struct LightTile {
    uint64_t lightMask;  // Bit N = light N affects this tile
    float zMin;          // Min depth in tile (for early-out)
    float zMax;          // Max depth in tile
};

/**
 * @brief Light data for CPU tile binning.
 */
struct TileLight {
    glm::vec3 position;
    float radius;
    glm::vec3 color;
    float intensity;
    glm::vec3 direction;  // For spot lights
    float spotAngle;      // 0 = point light, >0 = spot light
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
