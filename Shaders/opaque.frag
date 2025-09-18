//
layout(location = 0) in vec2 uv;
layout(location = 1) in mat3 TBN;        // Uses locations 1, 2, 3
layout(location = 4) in vec3 worldPos;   // Moved to location 4
layout(location = 5) in flat uint materialId;  // Moved to location 5

// Output
layout(location = 0) out vec4 outColor;

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

// Push constants using buffer references
layout(push_constant) uniform RenderConstants {
    mat4 viewProj;           // bytes 0-63
    vec3 cameraPos;          // bytes 64-75
    uint _pad;               // bytes 76-79
    TransformBuffer bufferTransforms;  // bytes 80-87
    DrawDataBuffer bufferDrawData;     // bytes 88-95
    MaterialBuffer bufferMaterials;    // bytes 96-103
} pc;

void main() {
    MaterialData mat = pc.bufferMaterials.materials[materialId];
    
    vec4 baseColor = mat.baseColorFactor;
    if (mat.baseColorTexture != 0) {
        vec4 textureSample = textureBindless2D(mat.baseColorTexture, 0, uv);
		baseColor *= textureSample;
    } 
    if (mat.emissiveFactorAlphaCutoff.w > 0.0) {
        if (baseColor.a < mat.emissiveFactorAlphaCutoff.w) {
            discard;
        }
    }
    vec3 finalColor = baseColor.rgb;
    vec3 emissive = mat.emissiveFactorAlphaCutoff.rgb;
    if (mat.emissiveTexture != 0) {
        emissive *= textureBindless2D(mat.emissiveTexture, 0, uv).rgb;
    }
    finalColor += emissive;
    outColor = vec4(finalColor, baseColor.a);
}