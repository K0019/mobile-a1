// Improved Horizontal Blur Shader
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform sampler2D inputImage;
layout(push_constant) uniform PushConstants {
    float step;  // Texel step size (1.0/width)
} pc;

void main() {
    // Use a 13-tap Gaussian kernel for higher quality blur
    // These weights approximate a Gaussian with sigma ≈ 3.0
    const float weights[13] = float[](
        0.002216, 0.008764, 0.026995, 0.064759, 0.120985, 0.176033, 
        0.199471, 
        0.176033, 0.120985, 0.064759, 0.026995, 0.008764, 0.002216
    );
    
    const float offsets[13] = float[](
        -6.0, -5.0, -4.0, -3.0, -2.0, -1.0, 
        0.0, 
        1.0, 2.0, 3.0, 4.0, 5.0, 6.0
    );
    
    vec4 color = vec4(0.0);
    float totalWeight = 0.0;
    
    // Improved brightness-sensitive blur
    // This helps preserve bright highlights better
    for (int i = 0; i < 13; i++) {
        vec2 sampleCoord = fragTexCoord + vec2(offsets[i] * pc.step, 0.0);
        
        // Sample with safe bounds check
        if (sampleCoord.x >= 0.0 && sampleCoord.x <= 1.0 && 
            sampleCoord.y >= 0.0 && sampleCoord.y <= 1.0) {
            
            // Sample with luminance-based weighting
            vec4 sampleC = texture(inputImage, sampleCoord);
            
            // Calculate luminance with additional emphasis on brighter areas
            float luminance = dot(sampleC.rgb, vec3(0.2126, 0.7152, 0.0722));
            
            // Adjust weight to preserve high intensity areas
            float brightness = 1.0 + luminance * 2.0; // More aggressive brightness preservation
            float adjustedWeight = weights[i] * brightness;
            
            color += sampleC * adjustedWeight;
            totalWeight += adjustedWeight;
        }
    }
    
    // Normalize
    if (totalWeight > 0.0) {
        color = color / totalWeight;
    }
    
    // Apply slight curve to enhance bloom appearance
    color.rgb = pow(color.rgb, vec3(0.9));
    
    outColor = color;
}