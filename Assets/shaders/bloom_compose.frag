#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D blurredGlow;

layout(push_constant) uniform PushConstants {
    float intensity;
} pc;

void main() {
    // Sample the blurred glow texture
    vec4 glow = texture(blurredGlow, fragTexCoord);
    
    // Apply gentler curve and lower intensity for subtler glow effect
    vec3 enhancedGlow = pow(glow.rgb, vec3(0.9)) * (pc.intensity * 0.6);
    
    // Threshold the glow so it only applies to brighter areas
    // This helps prevent washing out the original trail
    float luminance = dot(enhancedGlow, vec3(0.2126, 0.7152, 0.0722));
    
    // Create a soft threshold to avoid harsh cutoffs
    float threshold = 0.05;
    float softThreshold = smoothstep(threshold, threshold + 0.1, luminance);
    
    // Scale by the threshold to diminish very faint glow areas
    vec3 thresholdedGlow = enhancedGlow * softThreshold;
    
    // Preserve original alpha from glow texture for proper blending
    // This ensures we don't apply glow where there shouldn't be any
    float alphaFactor = glow.a * 0.8; // Further reduce overall alpha
    
    // Output with preserved alpha for better blending with the original scene
    outColor = vec4(thresholdedGlow, alphaFactor);
}