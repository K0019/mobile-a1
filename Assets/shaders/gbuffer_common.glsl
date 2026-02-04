// G-Buffer Common Definitions
// Shared bindings, structs, and snippets for all gbuffer (deferred) shaders
#pragma once
#include "shader_snippets.glsl"

// =============================================================================
// Bind Groups
// =============================================================================
group Frame = 0;
group Material = 1;
group Dynamic = 2;

// =============================================================================
// Frame UBO (Set 0) - Deferred rendering version (minimal)
// =============================================================================
bindings(Frame, start=0) {
  uniform(std140) FrameUBO {
    mat4 viewProj;
  } frame;
}

// =============================================================================
// Material Bind Group (Set 1)
// =============================================================================
bindings(Material, start=0) {
  uniform(std140) MaterialConstants {
    uvec4 packed;
  } material;
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_metallicRoughness;
  texture sampler2D u_emissive;
  texture sampler2D u_occlusion;
}

// =============================================================================
// Shared Varyings for G-Buffer shaders
// =============================================================================
struct Varyings {
  vec3 worldPos;
  vec3 normal;
  vec3 tangent;
  float bitangentSign;
  vec2 uv;
  float objectIdF;  // For object picking (float to avoid flat requirement)
};

// =============================================================================
// G-Buffer Output
// =============================================================================
struct FragOut {
  vec4 albedo;       // location 0: SRGB color
  vec4 normal;       // location 1: octahedral encoded normal (RG16F)
  vec4 materialData; // location 2: roughness, metallic, ao, emissive
  uint visibility;   // location 3: object ID for picking
};

// =============================================================================
// G-Buffer Fragment Logic Snippet
// =============================================================================
snippet GBufferFragment {
    // Process fragment and write to G-buffer outputs
    void writeGBuffer(vec3 N, vec2 uv, float objectIdF,
                      out vec4 outAlbedo, out vec4 outNormal,
                      out vec4 outMaterialData, out uint outVisibility) {
        // Unpack material constants
        vec4 baseColor;
        float roughnessConst, metallicConst, emissiveFactor, alphaCutoff;
        unpackMaterialConstants(material.packed, baseColor, roughnessConst, metallicConst, emissiveFactor, alphaCutoff);

        // Sample albedo and apply base color
        vec4 albedo = texture(u_albedo, uv) * baseColor;

        // Alpha cutoff testing
        if (albedo.a < alphaCutoff) {
            discard;
        }

        // Sample material textures
        vec2 mr = texture(u_metallicRoughness, uv).bg;  // B=metallic, G=roughness (glTF)
        float metallic = mr.x;
        float roughness = mr.y;
        float ao = texture(u_occlusion, uv).r;
        float emissive = sampleEmissiveIntensity(u_emissive, uv, emissiveFactor);

        // Write outputs
        outAlbedo = albedo;
        outNormal = vec4(encodeNormalOctahedral(N), 0.0, 1.0);
        outMaterialData = vec4(roughness, metallic, ao, emissive);
        outVisibility = uint(objectIdF + 0.5) + 1u;  // +1 so 0 = background
    }
}
