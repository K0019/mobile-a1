#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

// Push constants for vignette settings
layout(push_constant) uniform VignetteSettings {
    vec3 vignetteColor;       // Color of the vignette (RGBA)
    float vignetteIntensity;  // Overall intensity (0.0 to 1.0)
    float vignetteRadius;     // How far it extends from corners (0.0 to 1.0)
    float vignetteSmoothness; // Edge softness (higher = softer transition)
} settings;

void main() {
    // Calculate distance from center (0.5, 0.5)
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(fragTexCoord, center);
    
    // Create vignette effect
    float vignette = smoothstep(settings.vignetteRadius, 
                               settings.vignetteRadius - settings.vignetteSmoothness, 
                               dist);
    
    // Apply intensity adjustment
    vignette = pow(vignette, settings.vignetteIntensity);
    
    // For direct application, output a color to be multiplied with the scene
    // White (1,1,1,1) means no effect, vignetteColor for full effect
    outColor = mix(vec4(1.0), settings.vignetteColor.rgbb, 1.0 - vignette);
}