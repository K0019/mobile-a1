#version 450

// Inputs from vertex shader
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragGlowColor;
layout(location = 2) in vec2 fragQuadPos;
layout(location = 3) in float fragAge;
layout(location = 4) in float fragSmoothing;
layout(location = 5) in float fragDecay;

// Output
layout(location = 0) out vec4 outColor;

void main() {
    // Get normalized distance from center of trail (0 to 1, where 1 is edge)
    float distFromCenter = abs(fragTexCoord.y * 2.0 - 1.0);
    
    // Apply anti-aliasing smoothstep to help with jagged edges
    // Especially helpful for horizontal trails where pixelation is more noticeable
    float antiAliasEdge = 0.99;
    float edgeSmooth = smoothstep(1.0, antiAliasEdge, distFromCenter);
    
    // Use a wider, gentler falloff curve to reduce harshness and jagged appearance
    float radialFalloff = pow(cos(min(distFromCenter * 0.9, 1.0) * 1.571) * 0.5 + 0.5, 1.8);
    
    // Age-based decay - more aggressive to reduce overall glow intensity
    float normalizedAge = min(fragAge, 1.0);
    float ageDecay = 1.0 - smoothstep(0.0, 0.8, normalizedAge * fragDecay);
    
    // Extra smoothing for horizontal trail segments
    float smoothingFactor = max(fragSmoothing * 4.0, 0.2); // Increase minimum smoothing
    float edgeSmoothing = smoothstep(1.0, max(0.0, 1.0 - smoothingFactor), distFromCenter);
    
    // Combine all factors with emphasis on smooth edges
    float glowIntensity = sqrt(radialFalloff * ageDecay) * edgeSmoothing * edgeSmooth;
    
    // Use a much stronger power falloff to reduce the glow's visual impact
    glowIntensity = pow(glowIntensity, 2.0); // More aggressive falloff
    
    // Further reduce intensity for very subtle glow
    float finalIntensity = 0.4; // Significantly reduced from previous 0.85
    
    // Apply extremely reduced alpha 
    float alpha = glowIntensity * fragGlowColor.a * 0.4; // Reduced from 0.7 to 0.4
    
    // Output with greatly reduced intensity and alpha
    outColor = vec4(fragGlowColor.rgb * finalIntensity, alpha);
}