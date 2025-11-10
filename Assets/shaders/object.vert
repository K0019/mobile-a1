#include <shaders/scene_structures.sp>
#include <shaders/vertex_compression.sp>

layout(push_constant) uniform PerFrameData {
  FrameConstantsBuffer frameConstants;
  TransformBuffer transforms;
  DrawDataBuffer drawData;
  MaterialBuffer materials;
  MeshDecompressionBuffer meshDecomp;
  OIT oit;
  LightingData lighting;
  uint textureIndices;
} pc;

layout(location = 0) in vec3 in_position_snorm; // SNORM16 [3]
layout(location = 1) in float in_normal_x; // SNORM16
layout(location = 2) in vec4 in_nt; // SNORM RGB10A2 (normal.y, tangent.xy, flags)
layout(location = 3) in vec2 in_uv_unorm; // UNORM16 [2]

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_worldPos;
layout(location = 3) out flat uint out_materialId;
layout(location = 4) out mat3 out_TBN;

void main() {
  DrawData dd = pc.drawData.dd[gl_BaseInstance];
  MeshDecompressionData decomp = pc.meshDecomp.data[dd.meshDecompIndex];

  uint actualMaterialId = dd.materialId & 0x7FFFFFFFu;

  vec3 localPosition;
  vec3 localNormal;
  vec3 localTangent;
  float handedness;
  vec2 localUV;

  localUV = in_uv_unorm * decomp.uv_scale + decomp.uv_min;
  // Decompress position from compressed vertex buffer
  localPosition = in_position_snorm * decomp.bbox_half_extent + decomp.bbox_center;

  // Decompress normal (hardware provided ny via in_nt.r)
  float nx = in_normal_x;
  float ny = in_nt.r;
  bool nz_negative = (in_nt.a > 0.0);
  float nz_sign = nz_negative ? -1.0 : 1.0;
  float nz = sqrt(max(0.001, 1.0 - nx * nx - ny * ny)) * nz_sign;
  localNormal = normalize(vec3(nx, ny, nz));

  // Decompress tangent using hardware-provided SNORM values.
  float tx = in_nt.g;
  float ty = in_nt.b;
  float tz = sqrt(max(0.001, 1.0 - tx * tx - ty * ty)); // Always positive
  localTangent = normalize(vec3(tx, ty, tz));

  // Extract handedness from alpha bits
  bool handedness_negative = (abs(in_nt.a) < 0.5);
  handedness = handedness_negative ? -1.0 : 1.0;

  // Transform to world space
  mat4 modelMatrix = pc.transforms.model[dd.transformId];
  mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

  vec3 worldPosition = (modelMatrix * vec4(localPosition, 1.0)).xyz;
  vec3 worldNormal = normalize(normalMatrix * localNormal);
  vec3 worldTangent = normalize(normalMatrix * localTangent);
  vec3 worldBitangent = normalize(cross(worldNormal, worldTangent) * handedness);

  out_TBN = mat3(worldTangent, worldBitangent, worldNormal);

  // Output
  out_uv = localUV;
  out_worldPos = worldPosition;
  out_normal = worldNormal;
  out_materialId = actualMaterialId; // Use unpacked material ID
  gl_Position = pc.frameConstants.constants.viewProj * vec4(worldPosition, 1.0);
}
