//
layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_uv_x;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in float in_uv_y;
layout(location = 4) in vec4 in_tangent;

layout(location = 0) out vec2 uv;
layout(location = 1) out mat3 TBN;        // Uses locations 1, 2, 3
layout(location = 4) out vec3 worldPos;   // Moved to location 4
layout(location = 5) out flat uint materialId;  // Moved to location 5

struct DrawData {
    uint transformId;
    uint materialId;
};

// Buffer reference declarations
layout(buffer_reference, std430) readonly buffer TransformBuffer {
    mat4 transforms[];
};

layout(buffer_reference, std430) readonly buffer DrawDataBuffer {
    DrawData drawData[];
};

struct MaterialData {
    vec4 baseColorFactor;
    vec4 metallicRoughnessNormalOcclusion; // metallic, roughness, normal scale, occlusion
    vec4 emissiveFactorAlphaCutoff;        // emissive.rgb, alphaCutoff
    uint baseColorTexture;
    uint normalTexture;
    uint metallicRoughnessTexture;
    uint emissiveTexture;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer {
    MaterialData materials[];
};

// Push constants using buffer references instead of uint64_t
layout(push_constant) uniform RenderConstants {
    mat4 viewProj;           // bytes 0-63
    vec3 cameraPos;          // bytes 64-75
    uint _pad;               // bytes 76-79
    TransformBuffer bufferTransforms;  // bytes 80-87
    DrawDataBuffer bufferDrawData;     // bytes 88-95
    MaterialBuffer bufferMaterials;    // bytes 96-103
} pc;

void main() {
    // Access draw data using gl_BaseInstance
    DrawData dd = pc.bufferDrawData.drawData[gl_BaseInstance];
    mat4 modelMatrix = pc.bufferTransforms.transforms[dd.transformId];
    
    vec4 worldPosition = modelMatrix * vec4(in_position, 1.0);
    gl_Position = pc.viewProj * worldPosition;
    
    // Pass through texture coordinates (flip Y for OpenGL->Vulkan)
    uv = vec2(in_uv_x, in_uv_y);
    worldPos = worldPosition.xyz;
    materialId = dd.materialId;

    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 worldNormal = normalize(normalMatrix * in_normal);
    vec3 worldTangent = normalize(normalMatrix * in_tangent.xyz);
    vec3 worldBitangent = normalize(cross(worldNormal, worldTangent) * in_tangent.w);

    TBN = mat3(worldTangent, worldBitangent, worldNormal);
}