// im3d_lines.geom (expands lines to screen-space quads)
#version 450

layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

layout(location = 0) in vec4 inColor[];
layout(location = 1) in float inSize[];

layout(push_constant) uniform PushConstants {
    mat4 viewProj;
    vec2 viewport;
} pc;

layout(location = 0) out vec4 outColor;

void main() {
    vec2 p0 = gl_in[0].gl_Position.xy / gl_in[0].gl_Position.w;
    vec2 p1 = gl_in[1].gl_Position.xy / gl_in[1].gl_Position.w;
    
    vec2 dir = normalize(p1 - p0);
    vec2 perp = vec2(-dir.y, dir.x);
    
    float thickness = max(inSize[0], inSize[1]) / pc.viewport.y;
    vec2 offset = perp * thickness;
    
    // Emit quad
    gl_Position = vec4((p0 - offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    outColor = inColor[0];
    EmitVertex();
    
    gl_Position = vec4((p0 + offset) * gl_in[0].gl_Position.w, gl_in[0].gl_Position.zw);
    outColor = inColor[0];
    EmitVertex();
    
    gl_Position = vec4((p1 - offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    outColor = inColor[1];
    EmitVertex();
    
    gl_Position = vec4((p1 + offset) * gl_in[1].gl_Position.w, gl_in[1].gl_Position.zw);
    outColor = inColor[1];
    EmitVertex();
    
    EndPrimitive();
}