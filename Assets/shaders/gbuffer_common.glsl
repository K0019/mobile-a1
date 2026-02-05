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
    uvec4 packed[2];  // 32 bytes of packed material data
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
  vec4 normal;       // location 1: octahedral encoded normal (RG), flags (B), reserved (A)
  vec4 materialData; // location 2: roughness, metallic, ao, emissive
  uint visibility;   // location 3: object ID for picking
};

