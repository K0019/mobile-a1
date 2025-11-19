#include <shaders/scene_structures.sp>
#include <shaders/vertex_compression.sp>

layout(push_constant) uniform PerFrameData {
    FrameConstantsBuffer frameConstants;
    TransformBuffer transforms;
    DrawDataBuffer drawData;
    uint unused1;
    MeshDecompressionBuffer meshDecomp;
    uint textureIndices;
} pc;

layout(location = 0) in vec3 in_position;
layout(location = 1) in uint in_uvPacked;         // unused
layout(location = 2) in uint in_normalPacked;    // unused
layout(location = 3) in uint in_tangentPacked;   // unused

layout(location = 0) flat out uint outDrawID;

void main()
{
    DrawData dd = pc.drawData.dd[gl_BaseInstance];

    mat4 modelMatrix = pc.transforms.model[dd.transformId];
    vec3 worldPosition = (modelMatrix * vec4(in_position, 1.0)).xyz;

    gl_Position = pc.frameConstants.constants.viewProj * vec4(worldPosition, 1.0);
    outDrawID = gl_DrawID;
}
