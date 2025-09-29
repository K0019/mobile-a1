// shaders/transparent.frag - Simplified demo-style transparent rendering
#include <shaders/scene_common.sp>

#extension GL_EXT_shader_16bit_storage : require

layout(location = 0) in vec2 uv;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 worldPos;
layout(location = 3) in flat uint materialId;
layout(location = 4) in mat3 TBN;

layout (early_fragment_tests) in;
layout (set = 0, binding = 2, r32ui) uniform uimage2D kTextures2DInOut[];

void main() {
    MaterialData mat = pc.materials.material[materialId];
    
    // Sample base color
    vec4 baseColor = mat.baseColorFactor;
    if (mat.baseColorTexture != 0) {
        baseColor *= SRGBtoLINEAR(textureBindless2D(mat.baseColorTexture, 0, uv));
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
    
    // OIT insertion with lighting applied
    float alpha = clamp(baseColor.a, 0.0, 1.0);
    bool isTransparent = alpha > 0.01;
    uint mask = 1 << gl_SampleID;
    if (isTransparent && !gl_HelperInvocation && ((gl_SampleMaskIn[0] & mask) == mask)) {
        uint index = atomicAdd(pc.oit.atomicCounter.numFragments, 1);
        if (index < pc.oit.maxOITFragments) {
            uint prevIndex = imageAtomicExchange(kTextures2DInOut[pc.oit.texHeadsOIT], ivec2(gl_FragCoord.xy), index);
            TransparentFragment frag;
            frag.color = f16vec4(litColor, alpha); // Store lit color
            frag.depth = gl_FragCoord.z;
            frag.next = prevIndex;
            pc.oit.oitLists.frags[index] = frag;
        }
    }
}