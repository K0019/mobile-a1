// ============================================================================
// Vertex Compression/Decompression Utilities
// Shared across vertex shaders and compute shaders
// ============================================================================

// ============================================================================
// Compressed Vertex Format (16 bytes)
// ============================================================================
// C++ struct CompressedVertex layout:
//   int16_t pos_x, pos_y, pos_z;  // Offset 0-5 (6 bytes)
//   int16_t normal_x;               // Offset 6-7 (2 bytes)
//   uint32_t packed;                // Offset 8-11 (4 bytes)
//   uint16_t uv_x, uv_y;           // Offset 12-15 (4 bytes)
//
// GLSL representation as uvec4 (struct CompressedVertex { uvec4 data; }):
//   data[0]: (pos_y << 16) | (pos_x & 0xFFFF)
//   data[1]: (normal_x << 16) | (pos_z & 0xFFFF)
//   data[2]: packed (uint32)
//   data[3]: (uv_y << 16) | (uv_x & 0xFFFF)

// NOTE: CompressedVertex struct is defined in scene_structures.sp
// We don't redefine CompressedVertexBuffer here to avoid conflicts

// ============================================================================
// MeshDecompressionData (must match C++ struct exactly)
// ============================================================================
struct MeshDecompressionData {
    vec3 bbox_center;
    float _pad0;
    vec3 bbox_half_extent;
    float _pad1;
    vec2 uv_min;
    vec2 uv_scale;
    uint padding[4];
};

layout(buffer_reference, std430) readonly buffer MeshDecompressionBuffer {
    MeshDecompressionData data[];
};

// ============================================================================
// Helper functions for custom SNORM packing (10-10-10-2 layout)
// ============================================================================

uint encodeSnorm10(float value) {
    float clamped = clamp(value, -1.0, 1.0);
    int encoded = int(round(clamped * 511.0));
    encoded = clamp(encoded, -511, 511);
    return uint(encoded & 0x3FF);
}

uint encodeSnorm2(float value) {
    if(value > 0.5)
        return 1u;
    if(value < -0.5)
        return 2u;
    return 0u;
}

// UNORM10 encoding/decoding for the new compression scheme
uint encodeUnorm10(float value) {
    float clamped = clamp(value, 0.0, 1.0);
    return uint(round(clamped * 1023.0)) & 0x3FFu;
}

float decodeUnorm10(uint value) {
    return float(value & 0x3FFu) / 1023.0;
}

int signExtend10(uint value) {
    int signedValue = int(value);
    if((signedValue & 0x200) != 0)
        signedValue |= ~0x3FF;
    return signedValue;
}

float decodeSnorm10(uint value) {
    int signedValue = signExtend10(value & 0x3FFu);
    return clamp(float(signedValue) / 511.0, -1.0, 1.0);
}

float decodeSnorm2(uint value) {
    uint bits = value & 0x3u;
    if(bits == 2u)
        return -1.0;
    if(bits == 1u)
        return 1.0;
    return 0.0;
}

uint packSnorm3x10_1x2(vec4 value) {
    vec3 vec = clamp(value.xyz, -1.0, 1.0);
    uint packed = encodeSnorm10(vec.x);
    packed |= encodeSnorm10(vec.y) << 10;
    packed |= encodeSnorm10(vec.z) << 20;
    packed |= encodeSnorm2(value.w) << 30;
    return packed;
}

// Pack ny, tx, ty as UNORM with nx_sign and handedness in alpha bits
uint packNormalTangentUnorm(float ny, float tx, float ty, float nx_sign, float handedness) {
    // Remap from [-1,1] to [0,1] for UNORM storage
    float ny_unorm = ny * 0.5 + 0.5;
    float tx_unorm = tx * 0.5 + 0.5;
    float ty_unorm = ty * 0.5 + 0.5;

    uint packed = encodeUnorm10(ny_unorm);
    packed |= encodeUnorm10(tx_unorm) << 10;
    packed |= encodeUnorm10(ty_unorm) << 20;

    // Alpha encodes nx_sign (bit 1) and handedness (bit 0)
    uint nx_sign_bit = (nx_sign < 0.0) ? 1u : 0u;
    uint handedness_bit = (handedness < 0.0) ? 1u : 0u;
    uint alpha_bits = (nx_sign_bit << 1) | handedness_bit;
    packed |= alpha_bits << 30;

    return packed;
}

vec4 unpackSnorm3x10_1x2(uint packed) {
    float x = decodeSnorm10(packed & 0x3FFu);
    float y = decodeSnorm10((packed >> 10) & 0x3FFu);
    float z = decodeSnorm10((packed >> 20) & 0x3FFu);
    float w = decodeSnorm2(packed >> 30);
    return vec4(x, y, z, w);
}

// ============================================================================
// Decompression Helper Functions
// ============================================================================

// Extract int16_t from packed uint32 and convert to float
float extractInt16Snorm(uint packed, uint offset) {
    // Extract 16 bits at offset (0 or 16)
    int raw = int((packed >> offset) & 0xFFFFu);
    // Sign-extend from 16-bit to 32-bit
    if ((raw & 0x8000) != 0) {
        raw |= 0xFFFF0000;
    }
    // Convert SNORM16 to float [-1, 1]
    return float(raw) / 32767.0;
}

// Decompress position from compressed vertex data
vec3 decompressPosition(uvec4 vertexData, MeshDecompressionData decomp) {
    // Extract position components from packed data
    // data[0] = (pos_y << 16) | pos_x
    // data[1] = (normal_x << 16) | pos_z
    float px = extractInt16Snorm(vertexData[0], 0);   // pos_x (lower 16 bits)
    float py = extractInt16Snorm(vertexData[0], 16);  // pos_y (upper 16 bits)
    float pz = extractInt16Snorm(vertexData[1], 0);   // pos_z (lower 16 bits)

    vec3 snorm = vec3(px, py, pz);
    return snorm * decomp.bbox_half_extent + decomp.bbox_center;
}

// Decompress normal from compressed vertex data
vec3 decompressNormal(uvec4 vertexData) {
    float nx_encoded = extractInt16Snorm(vertexData[1], 16);  // |nx| with nz_sign in sign bit
    uint alpha_bits = (vertexData[2] >> 30) & 0x3u;

    // Extract nx_sign from alpha bit 1
    bool nx_negative = (alpha_bits & 2u) != 0u;
    float nx_sign = nx_negative ? -1.0 : 1.0;

    // Extract nz_sign from the sign of the encoded normal_x
    float nz_sign = (nx_encoded < 0.0) ? -1.0 : 1.0;

    // Reconstruct nx: magnitude from |nx_encoded|, sign from nx_sign
    float nx = abs(nx_encoded) * nx_sign;

    // Decode ny from UNORM and remap from [0,1] to [-1,1]
    float ny_unorm = decodeUnorm10(vertexData[2] & 0x3FFu);
    float ny = ny_unorm * 2.0 - 1.0;

    float nz = sqrt(max(0.001, 1.0 - nx * nx - ny * ny)) * nz_sign;

    return normalize(vec3(nx, ny, nz));
}

// Decompress UV from compressed vertex data
vec2 decompressUV(uvec4 vertexData, MeshDecompressionData decomp) {
    // Extract UV from data[3]: (uv_y << 16) | uv_x
    float u = float(vertexData[3] & 0xFFFFu) / 65535.0;
    float v = float((vertexData[3] >> 16) & 0xFFFFu) / 65535.0;

    return vec2(u, v) * decomp.uv_scale + decomp.uv_min;
}

// Decompress tangent from compressed vertex data
vec3 decompressTangent(uvec4 vertexData) {
    // Decode tx, ty from UNORM and remap from [0,1] to [-1,1]
    float tx_unorm = decodeUnorm10((vertexData[2] >> 10) & 0x3FFu);
    float ty_unorm = decodeUnorm10((vertexData[2] >> 20) & 0x3FFu);
    float tx = tx_unorm * 2.0 - 1.0;
    float ty = ty_unorm * 2.0 - 1.0;
    float tz = sqrt(max(0.001, 1.0 - tx*tx - ty*ty));  // Always positive

    return normalize(vec3(tx, ty, tz));
}

// Get tangent handedness from compressed vertex data
float getTangentHandedness(uvec4 vertexData) {
    uint alpha_bits = (vertexData[2] >> 30) & 0x3u;
    // Handedness is in bit 0
    bool handedness_negative = (alpha_bits & 1u) != 0u;
    return handedness_negative ? -1.0 : 1.0;
}

// ============================================================================
// Compression Helper Functions (for compute shaders)
// ============================================================================

// Encode float [-1, 1] to SNORM16
int encodeSnorm16(float value) {
    value = clamp(value, -1.0, 1.0);
    return int(value * 32767.0);
}

// Encode float [0, 1] to UNORM16
uint encodeUnorm16(float value) {
    value = clamp(value, 0.0, 1.0);
    return uint(value * 65535.0);
}

// Pack normal.y, tangent.xy, and sign bits into RGB10A2 UNORM
uint packNormalTangentSigns(float ny, float tx, float ty, float nx_sign, float handedness) {
    return packNormalTangentUnorm(ny, tx, ty, nx_sign, handedness);
}

// Compress a full vertex back to CompressedVertex format
// Input: position, normal, tangent (vec4, w=handedness), UV
// Output: uvec4 packed data
uvec4 compressVertex(vec3 position, vec3 normal, vec4 tangent, vec2 uv, MeshDecompressionData decomp) {
    uvec4 result;

    // Compress position to mesh-local space
    vec3 pos_local = (position - decomp.bbox_center) / decomp.bbox_half_extent;
    int pos_x = encodeSnorm16(pos_local.x);
    int pos_y = encodeSnorm16(pos_local.y);
    int pos_z = encodeSnorm16(pos_local.z);

    // Normalize normal
    vec3 n = normalize(normal);

    // Store |nx| in normal_x, with nz_sign encoded in its sign bit
    float nx_magnitude = abs(n.x);
    float nx_sign = (n.x < 0.0) ? -1.0 : 1.0;
    float nz_sign = (n.z < 0.0) ? -1.0 : 1.0;
    float nx_with_nz_sign = nx_magnitude * nz_sign;
    int normal_x = encodeSnorm16(nx_with_nz_sign);

    // Normalize tangent and handle handedness
    vec3 t = normalize(tangent.xyz);
    float handedness = tangent.w;

    // Flip tangent if tz < 0 to force tz positive
    if (t.z < 0.0) {
        t = -t;
        handedness = -handedness;
    }

    // Pack ny, tx, ty as UNORM, with nx_sign and handedness in alpha bits
    uint packed = packNormalTangentSigns(n.y, t.x, t.y, nx_sign, handedness);

    // Compress UV to mesh-local space
    vec2 uv_local = (uv - decomp.uv_min) / decomp.uv_scale;
    uint uv_x = encodeUnorm16(uv_local.x);
    uint uv_y = encodeUnorm16(uv_local.y);

    // Pack into uvec4 format
    result[0] = (uint(pos_y & 0xFFFF) << 16) | uint(pos_x & 0xFFFF);
    result[1] = (uint(normal_x & 0xFFFF) << 16) | uint(pos_z & 0xFFFF);
    result[2] = packed;
    result[3] = (uv_y << 16) | uv_x;

    return result;
}

// Uncompressed vertex structure for animation processing
struct Vertex {
    vec3 position;
    vec3 normal;
    vec4 tangent;  // xyz = tangent direction, w = handedness
    vec2 uv;
};

// Decompress a full vertex
Vertex decompressVertex(uvec4 vertexData, MeshDecompressionData decomp) {
    Vertex v;
    v.position = decompressPosition(vertexData, decomp);
    v.normal = decompressNormal(vertexData);
    v.tangent.xyz = decompressTangent(vertexData);
    v.tangent.w = getTangentHandedness(vertexData);
    v.uv = decompressUV(vertexData, decomp);
    return v;
}

// Compress a full vertex
uvec4 compressVertex(Vertex v, MeshDecompressionData decomp) {
    return compressVertex(v.position, v.normal, v.tangent, v.uv, decomp);
}
