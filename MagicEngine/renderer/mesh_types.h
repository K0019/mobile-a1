/**
 * @file mesh_types.h
 * @brief Common mesh data types shared by mesh storage systems.
 *
 * This file contains vertex formats and bounds structures used by both
 * GfxMeshStorage and ChunkedMeshStorage.
 */

#pragma once

#include "gfx_types.h"  // For VertexPosition, VertexAttributes
#include <glm/glm.hpp>

namespace gfx {

// ============================================================================
// Vertex Formats
// ============================================================================

/**
 * @brief Full vertex format for mesh import (48 bytes).
 * This matches the existing Vertex struct in gpu_data.h
 */
struct FullVertex {
    glm::vec3 position;   // 12 bytes
    float uv_x;           // 4 bytes
    glm::vec3 normal;     // 12 bytes
    float uv_y;           // 4 bytes
    glm::vec4 tangent;    // 16 bytes (xyz = tangent, w = handedness)

    FullVertex() = default;
    FullVertex(const glm::vec3& pos, const glm::vec3& n, const glm::vec2& uv)
        : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y)
        , tangent(0.0f, 0.0f, 0.0f, 1.0f) {}
    FullVertex(const glm::vec3& pos, const glm::vec3& n, const glm::vec2& uv, const glm::vec4& tan)
        : position(pos), uv_x(uv.x), normal(n), uv_y(uv.y), tangent(tan) {}

    glm::vec2 getUV() const { return glm::vec2(uv_x, uv_y); }
};
static_assert(sizeof(FullVertex) == 48, "FullVertex must be 48 bytes");

// ============================================================================
// Mesh Bounds
// ============================================================================

/**
 * @brief Mesh bounds for frustum culling.
 */
struct MeshBounds {
    glm::vec3 center;
    float radius;
    glm::vec3 aabbMin;
    glm::vec3 aabbMax;
};

} // namespace gfx
