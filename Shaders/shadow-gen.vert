#version 450

layout(location = 0) in vec2 inStart;
layout(location = 1) in vec2 inEnd;

layout(location = 0) out vec4 vsEdge;

void main() {
    // Generate full-screen quad (covering entire shadow map)
    // gl_VertexIndex: 0=BL, 1=BR, 2=TL, 3=TR (using triangle strip)
    float x = (gl_VertexIndex & 1) == 0 ? -1.0 : 1.0;
    float y = (gl_VertexIndex & 2) == 0 ? -1.0 : 1.0;
    
    // Position vertex at corner of screen-space quad
    gl_Position = vec4(x, y, 0.0, 1.0);
    
    // Pass shadow caster data to fragment shader
    vsEdge = vec4(inStart, inEnd);
}