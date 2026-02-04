// WBOIT (Weighted Blended Order-Independent Transparency) Common Definitions
// Shared bindings and snippets for transparent object rendering
#pragma once
#include "shader_snippets.glsl"

// =============================================================================
// Bind Groups
// =============================================================================
group Frame = 0;
group Material = 1;
group Dynamic = 2;
group Lights = 3;

// =============================================================================
// Frame UBO (Set 0) - Forward rendering version (includes camera pos)
// =============================================================================
bindings(Frame, start=0) {
  uniform(std140) FrameUBO {
    mat4 viewProj;
    vec4 cameraPos;  // xyz = camera position, w = unused
  } frame;
}

// =============================================================================
// Material Bind Group (Set 1) - Same layout as gbuffer
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
// Dynamic Bind Group (Set 2) - Same layout as gbuffer (for shared bind groups)
// =============================================================================
bindings(Dynamic, start=0) {
  uniform(std140) TransformUBO {
    mat4 model;
    uint objectId;
    uint flags;
    uint _pad1;
    uint _pad2;
  } transform;
  uniform(std140) BoneMatricesUBO {
    mat4 bones[256];
  } skin;
}

// =============================================================================
// Light UBO (Set 3) - Forward lighting data
// Each light is 4 vec4s: data0(type,pos), data1(dir,intensity), data2(color,radius), data3(reserved)
// =============================================================================
bindings(Lights, start=0) {
  uniform(std140) LightUBO {
    vec4 ambientColor;
    vec4 cameraPos;      // xyz = camera position (duplicated for convenience)
    mat4 invViewProj;
    ivec4 lightCounts;   // x = numLights, yzw = unused
    vec4 screenInfo;     // xy = screen dimensions, zw = 1.0/screen dimensions
    vec4 lights[128];    // 32 lights * 4 vec4s each
  } ubo;
}

// =============================================================================
// WBOIT Varyings (same as gbuffer but without objectIdF)
// =============================================================================
struct Varyings {
  vec3 worldPos;
  vec3 normal;
  vec3 tangent;
  float bitangentSign;
  vec2 uv;
};

// =============================================================================
// WBOIT Accumulation Output
// =============================================================================
struct FragOutAccum {
  vec4 accumulation;  // location 0: premultiplied color * weight, alpha * weight
};

// =============================================================================
// WBOIT Pick Output (for transparent object picking)
// =============================================================================
struct FragOutPick {
  uint visibility;  // location 0: object ID
};

// =============================================================================
// WBOIT Fragment Logic Snippet
// =============================================================================
snippet WBOITFragment {
    // Process WBOIT fragment with forward lighting and output weighted accumulation
    void writeWBOIT(vec3 worldPos, vec3 N, vec2 uv,
                    out vec4 outAccumulation) {
        // Unpack material
        vec4 baseColor = unpackBaseColor(material.packed);
        vec4 albedo = texture(u_albedo, uv) * baseColor;
        float alpha = albedo.a;

        // Discard fully transparent fragments
        if (alpha < 0.01) {
            discard;
        }

        // Material properties
        vec2 mr = texture(u_metallicRoughness, uv).bg;
        float roughness = mr.y;
        float specPower = mix(8.0, 128.0, 1.0 - roughness);

        // Lighting calculation
        vec3 viewDir = normalize(ubo.cameraPos.xyz - worldPos);
        vec3 totalDiffuse = vec3(0.0);
        vec3 totalSpecular = vec3(0.0);

        int numLights = ubo.lightCounts.x;
        for (int i = 0; i < numLights && i < 32; ++i) {
            int base = i * 4;
            vec4 data0 = ubo.lights[base + 0];
            vec4 data1 = ubo.lights[base + 1];
            vec4 data2 = ubo.lights[base + 2];

            int lightType = int(data0.x);
            vec3 lightPos = data0.yzw;
            vec3 lightDir = data1.xyz;
            float intensity = data1.w;
            vec3 lightColor = data2.rgb;
            float radius = data2.a;

            calculateLight(lightType, lightPos, lightDir, intensity, lightColor, radius,
                           worldPos, N, viewDir, specPower, roughness,
                           totalDiffuse, totalSpecular);
        }

        // Final lit color
        vec3 ambient = ubo.ambientColor.rgb;
        vec3 litColor = albedo.rgb * (ambient + totalDiffuse) + totalSpecular;

        // For fully opaque fragments, use maximum weight (ensures solid appearance)
        if (alpha >= 0.99) {
            float maxWeight = 3000.0;
            outAccumulation = vec4(litColor * maxWeight, maxWeight);
            return;
        }

        // WBOIT weight function
        float z = gl_FragCoord.z;
        float weight = calculateWBOITWeight(z, alpha);

        // Output weighted accumulation
        outAccumulation = vec4(litColor * alpha * weight, alpha * weight);
    }
}

// =============================================================================
// Transparent Pick Fragment Snippet
// =============================================================================
snippet TransparentPickFragment {
    // Simple pick pass for transparent objects
    void writeTransparentPick(vec2 uv, uint objectId, out uint outVisibility) {
        vec4 baseColor = unpackBaseColor(material.packed);
        vec4 albedo = texture(u_albedo, uv) * baseColor;

        if (albedo.a < 0.01) {
            discard;
        }

        outVisibility = objectId + 1u;  // 0 reserved for background
    }
}
