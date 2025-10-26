// shaders/enhanced_opaque.frag
#include <shaders/scene_common.sp>

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in flat uint materialId;
layout(location = 4) in mat3 TBN;

layout(location = 0) out vec4 outColor;

void main() {
MaterialData mat = pc.materials.material[materialId];
    
    // Sample base color
    vec4 baseColor = mat.baseColorFactor;
    if (mat.baseColorTexture != 0) {
        baseColor *= SRGBtoLINEAR(textureBindless2D(mat.baseColorTexture, 0, uv));
    }
    
    uint alphaMode = mat.flags & 0x3u;
    // Alpha testing for MASK mode
    if (alphaMode == 1u) { // ALPHA_MODE_MASK
        float threshold = mat.emissiveFactorAlphaCutoff.w;
        runAlphaTest(baseColor.a, threshold);
    }
    else if (alphaMode == 0u) { // ALPHA_MODE_OPAQUE  
        baseColor.a = 1.0;
    }
    
    // Sample material properties
    vec4 metallicRoughnessNormalOcclusion = mat.metallicRoughnessNormalOcclusion;
    if (mat.metallicRoughnessTexture != 0) {
        vec4 mrSample = textureBindless2D(mat.metallicRoughnessTexture, 0, uv);
        metallicRoughnessNormalOcclusion.xy *= mrSample.bg; // B = metallic, G = roughness
    }
    
    float metallic = metallicRoughnessNormalOcclusion.x;
    float roughness = metallicRoughnessNormalOcclusion.y;
    float occlusion = metallicRoughnessNormalOcclusion.w; 
    
    if (mat.occlusionTexture != 0) {
        occlusion *= textureBindless2D(mat.occlusionTexture, 0, uv).r;
    }

    // Calculate normal
    vec3 finalNormal = normalize(normal);
    if (mat.normalTexture != 0) {
        vec3 normalSample = textureBindless2D(mat.normalTexture, 0, uv).rgb;
        finalNormal = perturbNormal(normalSample, TBN);
    }
    
    vec3 viewDir = normalize(pc.cameraPos.xyz - worldPos);
    
    // NEW: Calculate clustered lighting for transparent fragment
    vec3 litColor = calculateLighting(worldPos, finalNormal, baseColor.rgb, metallic, roughness, viewDir, occlusion);
    
    // Add emissive
    vec3 emissive = mat.emissiveFactorAlphaCutoff.rgb;
    if (mat.emissiveTexture != 0) {
        emissive *= SRGBtoLINEAR(textureBindless2D(mat.emissiveTexture, 0, uv)).rgb;
    }
    litColor += emissive;
    
    outColor = vec4(litColor, baseColor.a);
}