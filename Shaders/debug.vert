#version 450

layout(location = 0) in vec2 inStart;
layout(location = 1) in vec2 inEnd;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(set = 0, binding = 0) uniform SceneUBO {
    mat4 projection;
} sceneData;

void main() {
    vec2 position = (gl_VertexIndex == 0) ? inStart : inEnd;
    gl_Position = sceneData.projection * vec4(position, 0.0, 1.0);
    fragColor = inColor;
}