#include <shaders/scene_structures.sp>
#include <shaders/vertex_compression.sp>

layout(constant_id = 0) const bool kUseSkinned = false;

layout(push_constant) uniform PerFrameData {
  FrameConstantsBuffer frameConstants;
  TransformBuffer transforms;
  DrawDataBuffer drawData;
  uint unused1;
  MeshDecompressionBuffer meshDecomp;
  uint textureIndices;
} pc;

// Vertex inputs (match object.vert)
layout(location = 0) in vec3 in_position_snorm; // SNORM16 [3]
layout(location = 1) in float in_normal_x; // SNORM16 (unused)
layout(location = 2) in vec4 in_nt; // RGB10A2 SNORM (unused)
layout(location = 3) in vec2 in_uv_unorm; // UNORM16 [2] (unused)

layout(location = 4) in vec3 in_skinned_position;
layout(location = 5) in uint in_skinned_uvPacked; // unused
layout(location = 6) in uint in_skinned_normalPacked; // unused
layout(location = 7) in uint in_skinned_tangentPacked; // unused

layout(location = 0) flat out uint outDrawID;

void main()
{
  DrawData dd = pc.drawData.dd[gl_BaseInstance];
  MeshDecompressionData decomp = pc.meshDecomp.data[dd.meshDecompIndex];

  vec3 position;

  if (kUseSkinned)
  {
    position = in_skinned_position;
  }
  else
  {
    position = in_position_snorm * decomp.bbox_half_extent + decomp.bbox_center;
  }

  // Transform to world space
  mat4 modelMatrix = pc.transforms.model[dd.transformId];
  vec3 worldPosition = (modelMatrix * vec4(position, 1.0)).xyz;

  // Transform to clip space
  gl_Position = pc.frameConstants.constants.viewProj * vec4(worldPosition, 1.0);

  // Output draw ID for visibility buffer (primitiveID available in fragment shader)
  outDrawID = gl_DrawID;
}
