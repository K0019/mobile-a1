/**
 * @file qtangent.h
 * @brief QTangent encoding/decoding utilities.
 *
 * QTangents encode the entire TBN (tangent-bitangent-normal) matrix in a single
 * quaternion, with bitangent handedness stored in the sign of w.
 *
 * Based on: https://developer.android.com/games/optimize/vertex-data-management
 *
 * Benefits:
 * - Mathematically robust: quaternion rotation is well-understood
 * - Compact: 8 bytes (4x int16_t SNORM) for entire TBN
 * - Correct transformation: quaternions compose correctly with model rotation
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include <cmath>

namespace gfx {
namespace qtangent {

/**
 * @brief Encode TBN (normal, tangent, handedness) into a quaternion.
 *
 * The quaternion represents the rotation from world-space basis vectors to the TBN frame.
 * Bitangent handedness is encoded in the sign of w:
 *   - Positive w: bitangent = cross(normal, tangent)
 *   - Negative w: bitangent = -cross(normal, tangent)
 *
 * @param normal    Unit normal vector
 * @param tangent   Unit tangent vector
 * @param handedness Bitangent handedness (+1.0 or -1.0)
 * @return Quaternion encoding the TBN frame
 */
inline glm::quat encode(const glm::vec3& normal, const glm::vec3& tangent, float handedness) {
    // Build TBN matrix: columns are tangent, bitangent, normal
    glm::vec3 n = glm::normalize(normal);
    glm::vec3 t = glm::normalize(tangent);

    // IMPORTANT: Always build a proper rotation matrix (det = +1)
    // Do NOT multiply bitangent by handedness here - that creates a reflection matrix
    // and quat_cast is undefined for reflection matrices (det = -1).
    // Handedness is encoded separately in the sign of qw.
    glm::vec3 b = glm::cross(n, t);

    // TBN matrix with tangent as X-axis, bitangent as Y-axis, normal as Z-axis
    // This matches the shader decode where qRotateZ gives normal
    glm::mat3 tbn(t, b, n);

    // Convert rotation matrix to quaternion
    glm::quat q = glm::quat_cast(tbn);
    q = glm::normalize(q);

    // Ensure w is positive for consistent decoding
    if (q.w < 0.0f) {
        q = -q;
    }

    // Bias w away from zero to avoid precision issues at singularity
    // Minimum representable in SNORM16: 1/32767 ≈ 3e-5
    const float bias = 1.0f / 32767.0f;
    if (q.w < bias) {
        // Renormalize while keeping w at minimum value
        float normFactor = std::sqrt(1.0f - bias * bias) /
                           std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z);
        q.x *= normFactor;
        q.y *= normFactor;
        q.z *= normFactor;
        q.w = bias;
    }

    // Encode handedness in w's sign (flip if negative handedness)
    if (handedness < 0.0f) {
        q = -q;
    }

    return q;
}

/**
 * @brief Pack a quaternion into 4x int16_t SNORM values.
 *
 * @param q Input quaternion
 * @param out_qx Output SNORM16 x component
 * @param out_qy Output SNORM16 y component
 * @param out_qz Output SNORM16 z component
 * @param out_qw Output SNORM16 w component (sign encodes handedness)
 */
inline void packSNORM16(const glm::quat& q,
                        int16_t& out_qx, int16_t& out_qy,
                        int16_t& out_qz, int16_t& out_qw) {
    out_qx = static_cast<int16_t>(glm::clamp(q.x * 32767.0f, -32767.0f, 32767.0f));
    out_qy = static_cast<int16_t>(glm::clamp(q.y * 32767.0f, -32767.0f, 32767.0f));
    out_qz = static_cast<int16_t>(glm::clamp(q.z * 32767.0f, -32767.0f, 32767.0f));
    out_qw = static_cast<int16_t>(glm::clamp(q.w * 32767.0f, -32767.0f, 32767.0f));
}

/**
 * @brief Unpack 4x int16_t SNORM values into a quaternion.
 *
 * @param qx SNORM16 x component
 * @param qy SNORM16 y component
 * @param qz SNORM16 z component
 * @param qw SNORM16 w component
 * @return Unpacked quaternion
 */
inline glm::quat unpackSNORM16(int16_t qx, int16_t qy, int16_t qz, int16_t qw) {
    return glm::quat(
        static_cast<float>(qw) / 32767.0f,  // w first in glm::quat constructor
        static_cast<float>(qx) / 32767.0f,
        static_cast<float>(qy) / 32767.0f,
        static_cast<float>(qz) / 32767.0f
    );
}

/**
 * @brief Decode a QTangent quaternion back to TBN vectors.
 *
 * @param q Input quaternion (from encode())
 * @param out_normal Output unit normal vector
 * @param out_tangent Output unit tangent vector
 * @param out_bitangent Output unit bitangent vector
 */
inline void decode(const glm::quat& q,
                   glm::vec3& out_normal, glm::vec3& out_tangent, glm::vec3& out_bitangent) {
    // Normalize to handle any precision loss
    glm::quat qn = glm::normalize(q);

    // Extract basis vectors from quaternion
    // These are the columns of the rotation matrix: q * basis_vector
    // Tangent = q * (1,0,0) = X-axis of rotation
    out_tangent = glm::vec3(
        1.0f - 2.0f * (qn.y * qn.y + qn.z * qn.z),
        2.0f * (qn.x * qn.y + qn.w * qn.z),
        2.0f * (qn.x * qn.z - qn.w * qn.y)
    );

    // Bitangent = q * (0,1,0) = Y-axis of rotation
    out_bitangent = glm::vec3(
        2.0f * (qn.x * qn.y - qn.w * qn.z),
        1.0f - 2.0f * (qn.x * qn.x + qn.z * qn.z),
        2.0f * (qn.y * qn.z + qn.w * qn.x)
    );

    // Normal = q * (0,0,1) = Z-axis of rotation
    out_normal = glm::vec3(
        2.0f * (qn.x * qn.z + qn.w * qn.y),
        2.0f * (qn.y * qn.z - qn.w * qn.x),
        1.0f - 2.0f * (qn.x * qn.x + qn.y * qn.y)
    );

    // Apply handedness correction (stored in sign of original w before normalize)
    // The sign of q.w indicates if bitangent should be flipped
    float sign = (q.w < 0.0f) ? -1.0f : 1.0f;
    out_bitangent *= sign;
}

/**
 * @brief Get just the normal from a QTangent (for simple normal-only use cases).
 *
 * @param q Input quaternion
 * @return Unit normal vector
 */
inline glm::vec3 decodeNormal(const glm::quat& q) {
    glm::quat qn = glm::normalize(q);
    // Normal = Z-axis of the quaternion rotation
    return glm::vec3(
        2.0f * (qn.x * qn.z + qn.w * qn.y),
        2.0f * (qn.y * qn.z - qn.w * qn.x),
        1.0f - 2.0f * (qn.x * qn.x + qn.y * qn.y)
    );
}

} // namespace qtangent
} // namespace gfx
