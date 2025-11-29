#extension GL_EXT_multiview : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable

// ============================================================================
// Point Light Shadow Vertex Shader
// Uses multiview to render all 6 cube faces in a single draw call
// ============================================================================

#include <shaders/scene_structures.sp>
#include <shaders/vertex_compression.sp>

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
  MeshDecompressionBuffer meshDecomp;
  uvec2 unused1; // oit - not needed for shadow pass
  uvec2 unused2; // lighting - not needed for shadow pass
  uvec2 unused3; // textureIndices - not needed for shadow pass
} pc;

// Material flags (must match CPU-side MaterialFlags enum)
const uint CAST_SHADOW_FLAG = 0x10;

// Vertex inputs (match depth_prepass.vert / object.vert)
layout(location = 0) in vec3 in_position_snorm; // SNORM16 [3]
layout(location = 1) in float in_normal_x; // SNORM16 (unused)
layout(location = 2) in vec4 in_nt; // RGB10A2 SNORM (unused)
layout(location = 3) in vec2 in_uv_unorm; // UNORM16 [2] (unused)

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

  // Get mesh decompression data
  MeshDecompressionData decomp = pc.meshDecomp.data[dd.meshDecompIndex];

  // Decompress position from SNORM16 format
  vec3 localPos = in_position_snorm * decomp.bbox_half_extent + decomp.bbox_center;

  // Get model transform
  mat4 model = pc.transforms.model[dd.transformId];

  // Transform to world space
  v_WorldPos = model * vec4(localPos, 1.0);

  // Use gl_ViewIndex from multiview extension to select the appropriate
  // view-projection matrix for this cube face
  // Face order: +X, -X, +Y, -Y, +Z, -Z (standard cube map convention)
  mat4 viewProj = pc.shadowFrameConstants.constants.viewProj[gl_ViewIndex];

  // Transform to clip space
  gl_Position = viewProj * v_WorldPos;
}
