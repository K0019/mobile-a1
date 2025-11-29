#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

// ============================================================================
// Point Light Shadow Fragment Shader
// Outputs linear distance from light to fragment, normalized by far plane
// ============================================================================

// Shadow-specific frame constants (must match vertex shader)
struct ShadowFrameConstants {
  mat4 viewProj[6];
  vec4 lightPos; // xyz = light position, w = unused
  float shadowNear;
  float shadowFar;
  uint shadowMapIndex;
  uint lightIndex;
};

layout(buffer_reference, std430) readonly buffer ShadowFrameConstantsBuffer {
  ShadowFrameConstants constants;
};

layout(push_constant) uniform ShadowPushConstants {
  ShadowFrameConstantsBuffer shadowFrameConstants;
  uvec2 unused0; // transforms
  uvec2 unused1; // drawData
  uvec2 unused2; // materials
  uvec2 unused3; // meshDecomp
  uvec2 unused4; // oit
  uvec2 unused5; // lighting
  uvec2 unused6; // textureIndices
} pc;

// Input from vertex shader
layout(location = 0) in vec4 v_WorldPos;

// Output linear depth to R16F cube map
layout(location = 0) out float out_LinearDepth;

void main() {
  // Calculate distance from light to this fragment in world space
  vec3 lightPos = pc.shadowFrameConstants.constants.lightPos.xyz;
  float lightDistance = length(v_WorldPos.xyz - lightPos);

  // Normalize by far plane to store in [0, 1] range
  // Values > 1.0 are clamped, representing objects beyond shadow range
  float shadowFar = pc.shadowFrameConstants.constants.shadowFar;
  out_LinearDepth = lightDistance / shadowFar;
}
