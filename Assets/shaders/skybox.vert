layout(push_constant) uniform SkyboxConstants {
    mat4 invViewProj;  // Precomputed inverse
    vec4 cameraPos;
    uint environmentTexture;
    float environmentIntensity;
} pc;

layout(location = 0) out vec4 clipPos;

void main() {
    // Generate fullscreen triangle (proven technique)
    vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    clipPos = vec4(uv * 2.0 - 1.0, 1.0, 1.0); // At far plane
    gl_Position = clipPos;
}