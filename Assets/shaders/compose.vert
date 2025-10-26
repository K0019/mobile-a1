#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec2 fragTexCoord;

void main() {
    // Transform from [-0.5, 0.5] to [-1, 1] with proper orientation
    vec2 position = inPosition * 2.0;
    
    // Important: No view or projection transforms needed
    gl_Position = vec4(position, 0.0, 1.0);
    
    // Direct texture coordinate mapping
    fragTexCoord = inTexCoord;
}