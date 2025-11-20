// ============================================================================
// Scene Common - Fragment Shader Functions
// Includes structures and adds texture/lighting functions
// ============================================================================

#include <shaders/scene_structures.sp>
#include <shaders/vertex_compression.sp>

layout(push_constant) uniform PerFrameData {
    FrameConstantsBuffer frameConstants;
    TransformBuffer transforms;
    DrawDataBuffer drawData;
    MaterialBuffer materials;
    MeshDecompressionBuffer meshDecomp;
    OIT oit;
    LightingData lighting;
    uint textureIndices;  // Reserved for future use
} pc;

uint getClusterIndex(vec3 fragCoord, vec3 viewPos) {
    uvec2 clusterXY = uvec2(fragCoord.xy / pc.lighting.screenDims *
                           vec2(pc.lighting.clusterDimX, pc.lighting.clusterDimY));

    float viewZ = -viewPos.z;
    float zSlice = log(viewZ / pc.lighting.zNear) / log(pc.lighting.zFar / pc.lighting.zNear);
    uint clusterZ = uint(clamp(zSlice * float(pc.lighting.clusterDimZ), 0.0,
                              float(pc.lighting.clusterDimZ - 1)));

    clusterXY = min(clusterXY, uvec2(pc.lighting.clusterDimX - 1, pc.lighting.clusterDimY - 1));

    return clusterZ * (pc.lighting.clusterDimX * pc.lighting.clusterDimY) +
           clusterXY.y * pc.lighting.clusterDimX + clusterXY.x;
}

vec3 perturbNormal(vec3 normalSample, mat3 TBN) {
    vec3 map = normalize(2.0 * normalSample - vec3(1.0));
    return normalize(TBN * map);
}

vec3 sampleIrradiance(vec3 N, uint irradianceMap) {
    if (irradianceMap == 0) return vec3(0.0);
    return textureBindlessCube(irradianceMap, 0, N).rgb;
}

vec3 samplePrefilter(vec3 R, float roughness, uint prefilterMap) {
    if (prefilterMap == 0) return vec3(0.0);
    float mipLevel = roughness * float(textureBindlessQueryLevelsCube(prefilterMap) - 1);
    return textureBindlessCubeLod(prefilterMap, 0, R, mipLevel).rgb;
}

vec2 sampleBRDF(float NdotV, float roughness, uint brdfLUT) {
    if (brdfLUT == 0) return vec2(0.0, 0.0);
    vec2 uv = vec2(NdotV, roughness);
    return textureBindless2D(brdfLUT, 0, uv).rg;
}

// Corrected PBR functions
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float distributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265359 * denom * denom;
    
    return num / denom;
}

float geometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float geometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = geometrySchlickGGX(NdotV, roughness);
    float ggx1 = geometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}


/* left out until I can actually load IBL
vec3 calculateLighting(vec3 worldPos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 viewDir, float occlusion) {
    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);

    // Calculate reflectance at normal incidence
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Transform world position to view space for clustering
    vec4 viewPos4 = pc.lighting.viewMatrix * vec4(worldPos, 1.0);
    vec3 viewPos = viewPos4.xyz;

    // Get cluster index
    uint clusterIndex = getClusterIndex(gl_FragCoord.xyz, viewPos);
    LightList lightList = pc.lighting.lightLists.lists[clusterIndex];

    // ===== DIRECT LIGHTING =====
    vec3 Lo = vec3(0.0);

    // Process each light affecting this cluster
    for (uint i = 0; i < lightList.count && i < 64; ++i) {
        uint lightIndex = pc.lighting.lightIndices.indices[lightList.offset + i];
        if (lightIndex >= pc.lighting.totalLightCount) continue;

        GPULight light = pc.lighting.lights.lights[lightIndex];

        vec3 L;
        float attenuation = 1.0;

        // Calculate light direction and attenuation
        if (light.type == 0) { // Directional Light
            L = normalize(-light.direction);
            attenuation = 1.0;
        }
        else if (light.type == 1) { // Point Light
            vec3 lightToFragment = worldPos - light.position;
            float distance = length(lightToFragment);
            L = normalize(-lightToFragment);
            attenuation = max(0.0, 1.0 - (distance / light.range));
            attenuation *= attenuation;
        }
        else if (light.type == 2) { // Spot Light
            vec3 lightToFragment = worldPos - light.position;
            float distance = length(lightToFragment);
            L = normalize(-lightToFragment);

            attenuation = max(0.0, 1.0 - (distance / light.range));
            attenuation *= attenuation;

            float spotFactor = dot(normalize(lightToFragment), light.direction);
            if (spotFactor > light.spotAngle) {
                float spotAttenuation = (spotFactor - light.spotAngle) / (1.0 - light.spotAngle);
                attenuation *= spotAttenuation;
            } else {
                attenuation = 0.0;
            }
        }

        if (attenuation > 0.0) {
            vec3 H = normalize(V + L);
            float NdotL = max(dot(N, L), 0.0);
            float NdotV = max(dot(N, V), 0.0);

            if (NdotL > 0.0) {
                // Cook-Torrance BRDF
                float NDF = distributionGGX(N, H, roughness);
                float G = geometrySmith(N, V, L, roughness);
                vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

                vec3 kS = F;
                vec3 kD = vec3(1.0) - kS;
                kD *= 1.0 - metallic; // Metals have no diffuse reflection

                vec3 numerator = NDF * G * F;
                float denominator = 4.0 * NdotV * NdotL + 0.0001;
                vec3 specular = numerator / denominator;

                vec3 diffuse = kD * albedo / 3.14159265359;

                vec3 radiance = light.color * attenuation;
                Lo += (diffuse + specular) * radiance * NdotL;
            }
        }
    }

    vec3 ambient = vec3(0.0);

    if (pc.irradianceTexture != 0 && pc.prefilteredTexture != 0 && pc.brdfLutTexture != 0) {
        vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

        vec3 kS = F;
        vec3 kD = 1.0 - kS;
        kD *= 1.0 - metallic;

        // Diffuse IBL
        vec3 irradiance = sampleIrradiance(N, pc.irradianceTexture);
        vec3 diffuse = irradiance * albedo;

        // Specular IBL
        vec3 R = reflect(-V, N);
        vec3 prefilteredColor = samplePrefilter(R, roughness, pc.prefilteredTexture);
        vec2 brdf = sampleBRDF(max(dot(N, V), 0.0), roughness, pc.brdfLutTexture);
        vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

        ambient = (kD * diffuse + specular) * pc.environmentIntensity;
        ambient *= occlusion;
    }

    vec3 color = ambient + Lo;

    return color;
}
*/

// Simplified Blinn-Phong lighting calculation
vec3 calculateLighting(vec3 worldPos, vec3 normal, vec3 albedo, float metallic, float roughness, vec3 viewDir, float occlusion) {
    vec3 N = normalize(normal);
    vec3 V = normalize(viewDir);

    // Transform world position to view space for clustering
    vec4 viewPos4 = pc.lighting.viewMatrix * vec4(worldPos, 1.0);
    vec3 viewPos = viewPos4.xyz;

    // Get cluster index
    uint clusterIndex = getClusterIndex(gl_FragCoord.xyz, viewPos);
    LightList lightList = pc.lighting.lightLists.lists[clusterIndex];

    // Material properties for Blinn-Phong
    vec3 ambient = albedo * 0.1; // Simple ambient term
    vec3 diffuseColor = albedo;
    vec3 specularColor = mix(vec3(0.04), albedo, metallic); // Use metallic to blend specular color
    float shininess = mix(32.0, 128.0, 1.0 - roughness); // Convert roughness to shininess

    vec3 result = ambient * occlusion;

    // Process each light affecting this cluster
    for (uint i = 0; i < lightList.count && i < 64; ++i) {
        uint lightIndex = pc.lighting.lightIndices.indices[lightList.offset + i];
        if (lightIndex >= pc.lighting.totalLightCount) continue;

        GPULight light = pc.lighting.lights.lights[lightIndex];

        vec3 L;
        float attenuation = 1.0;

        // Calculate light direction and attenuation
        if (light.type == 0) { // Directional Light
            L = normalize(-light.direction);
            attenuation = 1.0;
        }
        else if (light.type == 1) { // Point Light
            vec3 lightToFragment = worldPos - light.position;
            float distance = length(lightToFragment);
            L = normalize(-lightToFragment);
            attenuation = max(0.0, 1.0 - (distance / light.range));
            attenuation *= attenuation; // Quadratic falloff
        }
        else if (light.type == 2) { // Spot Light
            vec3 lightToFragment = worldPos - light.position;
            float distance = length(lightToFragment);
            L = normalize(-lightToFragment);

            attenuation = max(0.0, 1.0 - (distance / light.range));
            attenuation *= attenuation;

            float spotFactor = dot(normalize(lightToFragment), light.direction);
            if (spotFactor > light.spotAngle) {
                float spotAttenuation = (spotFactor - light.spotAngle) / (1.0 - light.spotAngle);
                attenuation *= spotAttenuation;
            } else {
                attenuation = 0.0;
            }
        }

        if (attenuation > 0.0) {
            float NdotL = max(dot(N, L), 0.0);

            if (NdotL > 0.0) {
                // Diffuse component
                vec3 diffuse = diffuseColor * NdotL;

                // Blinn-Phong specular component
                vec3 H = normalize(L + V); // Halfway vector
                float NdotH = max(dot(N, H), 0.0);
                float specular = pow(NdotH, shininess);
                vec3 spec = specularColor * specular;

                // Combine diffuse and specular with light color and attenuation
                vec3 lightContribution = (diffuse + spec) * light.color * attenuation;
                result += lightContribution;
            }
        }
    }

    return result;
}

vec4 SRGBtoLINEAR(vec4 srgbIn) {     vec3 linOut = pow(srgbIn.xyz, vec3(2.2));     return vec4(linOut, srgbIn.a); }

// Enhanced alpha test with anti-aliasing
void runAlphaTest(float alpha, float alphaThreshold) {
    if (alphaThreshold > 0.0) {
        mat4 thresholdMatrix = mat4(
            1.0/17.0,  9.0/17.0,  3.0/17.0, 11.0/17.0,
            13.0/17.0, 5.0/17.0, 15.0/17.0,  7.0/17.0,
            4.0/17.0, 12.0/17.0,  2.0/17.0, 10.0/17.0,
            16.0/17.0, 8.0/17.0, 14.0/17.0,  6.0/17.0
        );

        alpha = clamp(alpha - 0.5 * thresholdMatrix[int(mod(gl_FragCoord.x, 4.0))][int(mod(gl_FragCoord.y, 4.0))], 0.0, 1.0);

        if (alpha < alphaThreshold)
            discard;
    }
}
