// Shared Shader Snippets
// Common helper functions used across all shader types (gbuffer, forward, etc.)
#pragma once

// =============================================================================
// Math Utilities
// =============================================================================
snippet MathUtils {
    // Adjoint matrix - computes transpose of cofactor matrix
    // Correctly transforms normals for ANY determinant (including negative/mirrored)
    // Faster than transpose(inverse()) - just 3 cross products
    mat3 adjoint(mat4 m) {
        return mat3(
            cross(m[1].xyz, m[2].xyz),
            cross(m[2].xyz, m[0].xyz),
            cross(m[0].xyz, m[1].xyz)
        );
    }
}

// =============================================================================
// Vertex Attribute Decoding (CompressedVertex format)
// =============================================================================
snippet VertexDecode {
    // Decode compressed normal and tangent from vertex attributes
    // a_normalX: lower 16 bits = SNORM16 |nx| with nz_sign in sign bit
    // a_packed: RGB10A2 [ny(10), tx(10), ty(10), (nx_sign<<1)|handedness(2)]
    void decodeNormalTangent(uint a_normalX, uint a_packed,
                             out vec3 localNormal, out vec3 localTangent, out float handedness) {
        // Decode normal X component
        int normal_x_snorm = int(a_normalX & 0xFFFFu);
        if (normal_x_snorm > 32767) normal_x_snorm -= 65536;  // Sign extend
        float normal_x_float = float(normal_x_snorm) / 32767.0;

        float nx_magnitude = abs(normal_x_float);
        float nz_sign = normal_x_float < 0.0 ? -1.0 : 1.0;

        // Unpack RGB10A2
        float ny = float(a_packed & 0x3FFu) / 1023.0 * 2.0 - 1.0;
        float tx = float((a_packed >> 10u) & 0x3FFu) / 1023.0 * 2.0 - 1.0;
        float ty = float((a_packed >> 20u) & 0x3FFu) / 1023.0 * 2.0 - 1.0;
        uint alpha = (a_packed >> 30u) & 0x3u;

        float nx_sign = ((alpha >> 1u) & 1u) != 0u ? -1.0 : 1.0;
        handedness = (alpha & 1u) != 0u ? -1.0 : 1.0;

        // Reconstruct normal
        float nx = nx_magnitude * nx_sign;
        float nz = sqrt(max(0.001, 1.0 - nx * nx - ny * ny)) * nz_sign;
        localNormal = normalize(vec3(nx, ny, nz));

        // Reconstruct tangent (tz always positive)
        float tz = sqrt(max(0.001, 1.0 - tx * tx - ty * ty));
        localTangent = normalize(vec3(tx, ty, tz));
    }
}

// =============================================================================
// Material Unpacking
// =============================================================================
snippet MaterialUnpack {
    // Material GPU flags (matches gfx_types.h MaterialGPUFlags)
    const uint MAT_FLAG_UNLIT = 1u;
    const uint MAT_FLAG_DOUBLE_SIDED = 2u;
    // Unpack material constants from packed uvec4[2] (32 bytes)
    // Layout (matches gfx_types.h PackedMaterial):
    //   packed[0].x: baseColor.rg (half2)
    //   packed[0].y: baseColor.ba (half2) - alpha for transparency
    //   packed[0].z: roughness, metallic (half2)
    //   packed[0].w: emissive, alphaCutoff (half2)
    //   packed[1].x: normalScale, occlusionStrength (half2)
    //   packed[1].y: emissiveColor.rg (half2)
    //   packed[1].z: emissiveColor.b, padding (half2)
    //   packed[1].w: flags (uint32)
    void unpackMaterialConstants(uvec4 packed0, uvec4 packed1,
                                  out vec4 baseColor,
                                  out float roughness,
                                  out float metallic,
                                  out float emissiveFactor,
                                  out float alphaCutoff,
                                  out float normalScale,
                                  out float occlusionStrength,
                                  out vec3 emissiveColor,
                                  out uint flags) {
        vec2 rg = unpackHalf2x16(packed0.x);
        vec2 ba = unpackHalf2x16(packed0.y);
        vec2 rm = unpackHalf2x16(packed0.z);
        vec2 ec = unpackHalf2x16(packed0.w);
        vec2 no = unpackHalf2x16(packed1.x);
        vec2 emRG = unpackHalf2x16(packed1.y);
        vec2 emB_ = unpackHalf2x16(packed1.z);

        baseColor = vec4(rg.x, rg.y, ba.x, ba.y);
        roughness = rm.x;
        metallic = rm.y;
        emissiveFactor = ec.x;
        alphaCutoff = ec.y;
        normalScale = no.x;
        occlusionStrength = no.y;
        emissiveColor = vec3(emRG.x, emRG.y, emB_.x);
        flags = packed1.w;
    }

    // Legacy unpack for backwards compatibility (first 16 bytes only)
    void unpackMaterialConstantsLegacy(uvec4 packed,
                                  out vec4 baseColor,
                                  out float roughness,
                                  out float metallic,
                                  out float emissiveFactor,
                                  out float alphaCutoff) {
        vec2 rg = unpackHalf2x16(packed.x);
        vec2 ba = unpackHalf2x16(packed.y);
        vec2 rm = unpackHalf2x16(packed.z);
        vec2 ec = unpackHalf2x16(packed.w);

        baseColor = vec4(rg.x, rg.y, ba.x, ba.y);
        roughness = rm.x;
        metallic = rm.y;
        emissiveFactor = ec.x;
        alphaCutoff = ec.y;
    }

    // Simple base color unpack (when full material properties not needed)
    vec4 unpackBaseColor(uvec4 packed) {
        vec2 rg = unpackHalf2x16(packed.x);
        vec2 ba = unpackHalf2x16(packed.y);
        return vec4(rg.x, rg.y, ba.x, ba.y);
    }

    // Extract just normalScale from the second packed uvec4 (for early normal mapping)
    float unpackNormalScale(uvec4 packed1) {
        vec2 no = unpackHalf2x16(packed1.x);
        return no.x;
    }
}

// =============================================================================
// Normal Mapping
// =============================================================================
snippet NormalMapping {
    // Build TBN matrix and apply normal map if present
    // Returns the final world-space normal
    // normalScale: scales the XY components of the normal map (1.0 = normal, <1 = flatter, >1 = stronger)
    vec3 applyNormalMapScaled(vec3 N, vec3 T, float bitangentSign, vec2 uv, sampler2D normalTex, float normalScale) {
        vec3 B = normalize(cross(N, T) * bitangentSign);

        // Check if normal map is non-flat (default flat normal is (0.5, 0.5, 1.0))
        vec3 normalSample = texture(normalTex, uv).rgb;
        bool hasNormalMap = abs(normalSample.x - 0.5) > 0.01 || abs(normalSample.y - 0.5) > 0.01;

        if (hasNormalMap) {
            mat3 TBN = mat3(T, B, N);
            vec2 normalRG = normalSample.xy * 2.0 - 1.0;
            normalRG *= normalScale;  // Apply normal scale
            float nz = sqrt(max(0.0, 1.0 - dot(normalRG, normalRG)));
            vec3 normalMap = normalize(vec3(normalRG, nz));
            N = normalize(TBN * normalMap);
        }

        return N;
    }

    // Legacy version without scale (defaults to 1.0)
    vec3 applyNormalMap(vec3 N, vec3 T, float bitangentSign, vec2 uv, sampler2D normalTex) {
        return applyNormalMapScaled(N, T, bitangentSign, uv, normalTex, 1.0);
    }
}

// =============================================================================
// Normal Encoding (Octahedral)
// =============================================================================
snippet NormalEncoding {
    // Encode world-space normal to octahedral representation (fits in RG16F)
    vec2 encodeNormalOctahedral(vec3 N) {
        vec3 n = N / (abs(N.x) + abs(N.y) + abs(N.z));
        // Fold lower hemisphere
        if (n.z < 0.0) {
            float nx = n.x;
            float ny = n.y;
            n.x = (1.0 - abs(ny)) * (nx >= 0.0 ? 1.0 : -1.0);
            n.y = (1.0 - abs(nx)) * (ny >= 0.0 ? 1.0 : -1.0);
        }
        return n.xy;
    }

    // Decode octahedral normal back to world-space
    vec3 decodeNormalOctahedral(vec2 e) {
        vec3 v = vec3(e.xy, 1.0 - abs(e.x) - abs(e.y));
        if (v.z < 0.0) {
            v.xy = (1.0 - abs(v.yx)) * vec2(v.x >= 0.0 ? 1.0 : -1.0, v.y >= 0.0 ? 1.0 : -1.0);
        }
        return normalize(v);
    }
}

// =============================================================================
// Emissive Sampling
// =============================================================================
snippet EmissiveSampling {
    // Sample emissive texture with RGB factor (industry standard: emissiveFactor * emissiveTexture)
    // Returns full RGB emissive contribution
    vec3 sampleEmissiveRGB(sampler2D emissiveTex, vec2 uv, vec3 emissiveColorFactor, float emissiveFactor) {
        vec3 texSample = texture(emissiveTex, uv).rgb;
        return texSample * emissiveColorFactor * emissiveFactor;
    }

    // Sample emissive and compute luminance intensity for G-buffer storage
    // Uses luminance weighting for grayscale emissive value
    float sampleEmissiveIntensity(sampler2D emissiveTex, vec2 uv, vec3 emissiveColorFactor, float emissiveFactor) {
        vec3 emissiveRGB = sampleEmissiveRGB(emissiveTex, uv, emissiveColorFactor, emissiveFactor);
        // Rec. 709 luminance weights
        return dot(emissiveRGB, vec3(0.2126, 0.7152, 0.0722));
    }

    // Legacy version with scalar factor only
    float sampleEmissiveIntensityLegacy(sampler2D emissiveTex, vec2 uv, float emissiveFactor) {
        vec3 emissiveColor = texture(emissiveTex, uv).rgb;
        return dot(emissiveColor, vec3(0.2126, 0.7152, 0.0722)) * emissiveFactor;
    }
}

// =============================================================================
// Forward Lighting (Blinn-Phong)
// Used by WBOIT transparent shaders and composite deferred pass
// =============================================================================
snippet ForwardLighting {
    // Calculate lighting contribution from a single light
    // lightType: 0 = directional, 1 = point
    void calculateLight(int lightType, vec3 lightPos, vec3 lightDir, float intensity,
                        vec3 lightColor, float radius,
                        vec3 worldPos, vec3 N, vec3 viewDir, float specPower, float roughness,
                        inout vec3 totalDiffuse, inout vec3 totalSpecular) {
        vec3 L;
        float attenuation = 1.0;

        if (lightType == 0) {
            // Directional light
            L = -normalize(lightDir);
        } else {
            // Point light
            vec3 toLight = lightPos - worldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 0.001);

            if (radius > 0.0) {
                attenuation = 1.0 - smoothstep(0.0, radius, dist);
                attenuation *= attenuation;
            } else {
                attenuation = 1.0 / (1.0 + dist * dist * 0.01);
            }
        }

        // Diffuse
        float NdotL = max(dot(N, L), 0.0);
        totalDiffuse += lightColor * NdotL * intensity * attenuation;

        // Specular (Blinn-Phong)
        vec3 halfVec = normalize(L + viewDir);
        float NdotH = max(dot(N, halfVec), 0.0);
        float spec = pow(NdotH, specPower) * (1.0 - roughness) * 0.5;
        totalSpecular += lightColor * spec * intensity * attenuation;
    }
}

// =============================================================================
// WBOIT Weight Function
// =============================================================================
snippet WBOITWeight {
    // Calculate WBOIT weight for order-independent transparency
    // Based on McGuire's weight function
    // z: gl_FragCoord.z (0 = near, 1 = far in Vulkan)
    // alpha: fragment opacity
    float calculateWBOITWeight(float z, float alpha) {
        // McGuire's weight function (simplified):
        // w(z, α) = α · clamp(scale / (ε + z^power), min, max)
        //
        // Weights span useful range:
        // - At z=0 (near): weight ≈ alpha * 3000
        // - At z=0.5: weight ≈ alpha * 750
        // - At z=1 (far): weight ≈ alpha * 10
        float depthWeight = 10.0 / (1e-5 + pow(z, 4.0) + pow(z * 5.0, 6.0) * 1e-5);
        return clamp(alpha * depthWeight, 0.01, 3000.0);
    }
}

// =============================================================================
// G-Buffer Fragment Processing
// Shared logic for all G-buffer shaders (static, skinned, morphed)
// =============================================================================
snippet GBufferFragment {
    // Process G-buffer fragment: unpack material, sample textures, write outputs
    // Returns true if fragment should be kept, false if discarded (alpha cutoff)
    // Note: Caller must handle discard since GLSL doesn't allow discard in functions
    bool processGBufferMaterial(
        vec3 inNormal, vec3 inTangent, float inBitangentSign, vec2 inUV,
        uvec4 matPacked0, uvec4 matPacked1,
        sampler2D albedoTex, sampler2D normalTex, sampler2D metallicRoughnessTex,
        sampler2D emissiveTex, sampler2D occlusionTex,
        out vec4 outAlbedo, out vec3 outNormal, out float outRoughness,
        out float outMetallic, out float outAO, out float outEmissive, out uint outFlags
    ) {
        // Unpack material constants (32 bytes)
        vec4 baseColor;
        float roughnessConst, metallicConst, emissiveFactor, alphaCutoff;
        float normalScale, occlusionStrength;
        vec3 emissiveColor;
        unpackMaterialConstants(matPacked0, matPacked1,
                                baseColor, roughnessConst, metallicConst, emissiveFactor, alphaCutoff,
                                normalScale, occlusionStrength, emissiveColor, outFlags);

        // Sample albedo and apply base color
        outAlbedo = texture(albedoTex, inUV) * baseColor;

        // Alpha cutoff testing - return false to signal discard
        if (outAlbedo.a < alphaCutoff) {
            return false;
        }

        // Compute world-space normal with normal mapping and scale
        outNormal = applyNormalMapScaled(normalize(inNormal), normalize(inTangent),
                                         inBitangentSign, inUV, normalTex, normalScale);

        // Sample material textures
        vec2 mr = texture(metallicRoughnessTex, inUV).bg;  // B=metallic, G=roughness (glTF)
        outMetallic = mr.x;
        outRoughness = mr.y;

        // Sample occlusion with strength
        float aoSample = texture(occlusionTex, inUV).r;
        outAO = mix(1.0, aoSample, occlusionStrength);

        // Sample emissive with RGB color factor
        outEmissive = sampleEmissiveIntensity(emissiveTex, inUV, emissiveColor, emissiveFactor);

        return true;
    }

    // Encode G-buffer outputs: normal (octahedral), flags, material data, visibility
    void encodeGBufferOutputs(
        vec4 albedo, vec3 N, float roughness, float metallic, float ao, float emissive,
        uint flags, float objectIdF,
        out vec4 outAlbedo, out vec4 outNormal, out vec4 outMaterialData, out uint outVisibility
    ) {
        float flagsEncoded = float(flags & 0xFFu) / 255.0;
        outAlbedo = albedo;
        outNormal = vec4(encodeNormalOctahedral(N), flagsEncoded, 1.0);
        outMaterialData = vec4(roughness, metallic, ao, emissive);
        outVisibility = uint(objectIdF + 0.5) + 1u;  // +1 so 0 = background
    }
}

// =============================================================================
// Transparent Object Picking
// Shared logic for transparent pick pass shaders
// =============================================================================
snippet TransparentPickFragment {
    // Process transparent pick fragment: alpha test and write visibility
    // Returns true if fragment should be kept, false if discarded
    bool processTransparentPick(vec2 uv, uvec4 matPacked0, sampler2D albedoTex,
                                uint objectId, out uint outVisibility) {
        vec4 baseColor = unpackBaseColor(matPacked0);
        vec4 albedo = texture(albedoTex, uv) * baseColor;

        if (albedo.a < 0.01) {
            return false;
        }

        outVisibility = objectId + 1u;  // 0 reserved for background
        return true;
    }
}
