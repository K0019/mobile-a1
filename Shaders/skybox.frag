layout(push_constant) uniform SkyboxConstants {
    mat4 invViewProj;
    vec4 cameraPos;
    uint environmentTexture;
    float environmentIntensity;
} pc;

layout(location = 0) in vec4 clipPos;
layout(location = 0) out vec4 outColor;

void main() {
    // Transform clip position back to world direction
    vec4 worldPos = pc.invViewProj * clipPos;
    vec3 worldDir = normalize(worldPos.xyz / worldPos.w);
    
    if (pc.environmentTexture == 0) {
    // Procedural atmospheric sky
        vec3 dir = normalize(worldDir);
        
        // Sky gradient from horizon to zenith
        float elevation = dir.y;
        float horizon = smoothstep(-0.1, 0.3, elevation);
        
        vec3 zenithColor = vec3(0.35, 0.45, 0.6);
        // Horizon: A lighter, slightly desaturated color where sky meets ground.
        vec3 horizonColor = vec3(0.6, 0.65, 0.7);
        // Nadir: A neutral cool grey. This is crucial for PBR, as it acts
        // as the "bounce light" from the ground, preventing black shadows.
        vec3 nadirColor = vec3(0.3, 0.3, 0.35);

        // 2. Blend between the colors based on the view direction's elevation.
        // The smoothstep creates a soft blend from 0 (horizon) to 1 (zenith/nadir).
        float blendFactor = smoothstep(0.0, 1.0, abs(dir.y));
        
        vec3 finalColor;
        if (dir.y > 0.0) {
            // Above the horizon: blend between horizon and zenith.
            finalColor = mix(horizonColor, zenithColor, blendFactor);
        } else {
            // Below the horizon: blend between horizon and nadir.
            finalColor = mix(horizonColor, nadirColor, blendFactor);
        }
        
        outColor = vec4(finalColor, 1.0);
    } else {
        vec3 envColor = textureBindlessCubeLod(pc.environmentTexture, 0, worldDir, 0.0).rgb;
        outColor = vec4(envColor * pc.environmentIntensity, 1.0);
    }
}