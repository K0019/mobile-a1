// im3d.vert
#version 450

layout(location = 0) in vec4 inPositionSize;  // xyz=pos, w=size
layout(location = 1) in vec4 inColor;

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec2 viewport;
} pc;

layout(location = 0) out vec4 outColor;
layout(location = 1) out float outSize;

void main() {
    gl_Position = pc.viewProj * vec4(inPositionSize.xyz, 1.0);
    outColor = inColor;
    outSize = inPositionSize.w;
}