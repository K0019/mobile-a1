#extension GL_EXT_multiview : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

// ============================================================================
// Point Light Shadow Vertex Shader (Skinned/Animated meshes)
// Uses multiview to render all 6 cube faces in a single draw call
// ============================================================================

#include <shaders/scene_structures.sp>

// Shadow-specific frame constants with per-face view-projection matrices
struct ShadowFrameConstants {
  mat4 viewProj[6]; // One view-projection matrix per cube face
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
  TransformBuffer transforms;
  DrawDataBuffer drawData;
  MaterialBuffer materials;
  uvec2 unused1; // meshDecomp - not needed for skinned
  uvec2 unused2; // oit - not needed for shadow pass
  uvec2 unused3; // lighting - not needed for shadow pass
  uvec2 unused4; // textureIndices - not needed for shadow pass
} pc;

// Material flags (must match CPU-side MaterialFlags enum)
const uint CAST_SHADOW_FLAG = 0x10;

// Skinned vertex inputs (uncompressed format)
layout(location = 0) in vec3 in_position;
layout(location = 1) in uint in_uvPacked;      // unused for shadow
layout(location = 2) in uint in_normalPacked;  // unused for shadow
layout(location = 3) in uint in_tangentPacked; // unused for shadow

// Output to fragment shader
layout(location = 0) out vec4 v_WorldPos;

void main() {
  // Get draw data for this instance
  DrawData dd = pc.drawData.dd[gl_BaseInstance];

  // Check shadow override in upper bits of objectId
  // Bits 30-31: 0 = use material, 1 = force off, 2 = force on
  uint shadowOverride = (dd.objectId >> 30) & 0x3;
  
  // Determine if this object casts shadows
  uint effectiveFlags = pc.materials.material[dd.materialId & 0x7FFFFFFF].flags;
  if (shadowOverride == 1) {
    effectiveFlags &= ~CAST_SHADOW_FLAG; // Force off
  } else if (shadowOverride == 2) {
    effectiveFlags |= CAST_SHADOW_FLAG; // Force on
  }

  if ((effectiveFlags & CAST_SHADOW_FLAG) == 0) {
    gl_Position = vec4(0.0, 0.0, -2.0, 1.0);
    return;
  }

  // Get model transform
  mat4 model = pc.transforms.model[dd.transformId];

  // Transform to world space (position already deformed by compute shader)
  v_WorldPos = model * vec4(in_position, 1.0);

  // Use gl_ViewIndex from multiview extension to select the appropriate
  // view-projection matrix for this cube face
  mat4 viewProj = pc.shadowFrameConstants.constants.viewProj[gl_ViewIndex];

  // Transform to clip space
  gl_Position = viewProj * v_WorldPos;
}
