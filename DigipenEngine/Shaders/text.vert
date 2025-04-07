#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;

layout(location = 2) in mat4 inInstanceMatrix;
layout(location = 6) in vec4 inInstanceColor;
layout(location = 7) in vec4 inTexCoords;
layout(location = 8) in uint inTexIndex;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;
layout(location = 2) flat out uint fragTexIndex;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 projection;
} sceneData;

void main() {
    vec4 worldPos = inInstanceMatrix * vec4(inPosition, 1.0, 1.0);
    gl_Position = sceneData.projection * worldPos;
    
    // Calculate texture coordinates based on input texture coordinates
    fragTexCoord = inTexCoords.xy + inTexCoord * inTexCoords.zw;
    fragColor = inInstanceColor; //test if the build edits proeprl
    fragTexIndex = inTexIndex;
}