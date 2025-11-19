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

layout(location = 0) in vec3 in_position;
layout(location = 1) in uint in_uvPacked;
layout(location = 2) in uint in_normalPacked;
layout(location = 3) in uint in_tangentPacked;

layout(location = 0) out vec2 out_uv;
layout(location = 1) out vec3 out_normal;
layout(location = 2) out vec3 out_worldPos;
layout(location = 3) out flat uint out_materialId;
layout(location = 4) out mat3 out_TBN;

vec3 unpackNormal(uint packed)
{
    vec4 unpacked = unpackSnorm3x10_1x2(packed);
    return normalize(unpacked.xyz);
}

vec4 unpackTangent(uint packed)
{
    vec4 unpacked = unpackSnorm3x10_1x2(packed);
    vec3 direction = normalize(unpacked.xyz);
    return vec4(direction, unpacked.w);
}

void main() {
    DrawData dd = pc.drawData.dd[gl_BaseInstance];

    uint actualMaterialId = dd.materialId & 0x7FFFFFFFu;

    out_uv = unpackUnorm2x16(in_uvPacked);

    vec3 localPosition = in_position;
    vec3 localNormal = unpackNormal(in_normalPacked);
    vec4 localTangent4 = unpackTangent(in_tangentPacked);

    mat4 modelMatrix = pc.transforms.model[dd.transformId];
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));

    vec3 worldPosition = (modelMatrix * vec4(localPosition, 1.0)).xyz;
    vec3 worldNormal = normalize(normalMatrix * localNormal);
    vec3 worldTangent = normalize(normalMatrix * localTangent4.xyz);
    vec3 worldBitangent = normalize(cross(worldNormal, worldTangent) * localTangent4.w);

    out_TBN = mat3(worldTangent, worldBitangent, worldNormal);
    out_worldPos = worldPosition;
    out_normal = worldNormal;
    out_materialId = actualMaterialId;

    gl_Position = pc.frameConstants.constants.viewProj * vec4(worldPosition, 1.0);
}
