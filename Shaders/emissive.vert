// Revised Glow Map Vertex Shader
#version 450

// Vertex attributes for the quad
layout(location = 0) in vec2 inPosition;  // -0.5 to 0.5 quad position

// Packed instance data for glow segments
layout(location = 1) in vec4 inPoints;      // (startX, startY, endX, endY)
layout(location = 2) in vec4 inPerps;       // (startPerpX, startPerpY, endPerpX, endPerpY)
layout(location = 3) in vec4 inWidthsAges;  // (startWidth, endWidth, startAge, endAge)
layout(location = 4) in vec4 inGlowColor;
layout(location = 5) in vec4 inGlowParams;  // (intensity, decay, smoothing, unused)

// Outputs to fragment shader
layout(location = 0) out vec2 fragTexCoord;     // For sampling patterns or textures
layout(location = 1) out vec4 fragGlowColor;    // Base glow color
layout(location = 2) out vec2 fragQuadPos;      // Quad position for edge calculations
layout(location = 3) out float fragAge;         // Interpolated age
layout(location = 4) out float fragSmoothing;   // Edge smoothing factor
layout(location = 5) out float fragDecay;       // Decay rate

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 projection;
} sceneData;

void main() {
    // Unpack trail data
    vec2 startPoint = inPoints.xy;
    vec2 endPoint = inPoints.zw;
    vec2 startJointPerp = inPerps.xy;
    vec2 endJointPerp = inPerps.zw;
    float startWidth = inWidthsAges.x;
    float endWidth = inWidthsAges.y;
    float startAge = inWidthsAges.z;
    float endAge = inWidthsAges.w;
    float intensity = inGlowParams.x;
    float decay = inGlowParams.y;
    float smoothing = inGlowParams.z;
    
    // Position along trail (0 to 1)
    float t = inPosition.x + 0.5;
    
    // Side multiplier (-1 for left edge, +1 for right edge)
    float side = sign(inPosition.y);
    
    // Calculate width at this position
    float width = mix(startWidth, endWidth, t);
    
    // Calculate world position along the trail centerline
    vec2 centerPos = mix(startPoint, endPoint, t);
    
    // Interpolate between start and end joint perpendicular directions
    // This is the key to smooth joint transitions
    vec2 perpDir = normalize(mix(startJointPerp, endJointPerp, t));
    
    // Apply the offset using the joint-aware perpendicular direction
    vec2 finalPos = centerPos + perpDir * (side * width * 0.5);
    
    // Set position with projection
    gl_Position = sceneData.projection * vec4(finalPos, 0.0, 1.0);
    
    // Pass parameters to fragment shader
    fragTexCoord = vec2(t, side * 0.5 + 0.5);  // Normalized position
    fragGlowColor = inGlowColor * intensity;
    fragQuadPos = vec2(t, side * 0.5);         // Store position for edge calculations
    fragAge = mix(startAge, endAge, t);        // Interpolated age
    fragSmoothing = smoothing;
    fragDecay = decay;
}