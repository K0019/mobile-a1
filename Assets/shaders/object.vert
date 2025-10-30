// shaders/unified_object.vert
layout(location = 0) in vec3 in_position;
layout(location = 1) in float in_uv_x;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in float in_uv_y;
layout(location = 4) in vec4 in_tangent;

layout(location = 0) out vec2 uv;
layout(location = 1) out vec3 normal;
layout(location = 2) out vec3 worldPos;
layout(location = 3) out flat uint materialId;
layout(location = 4) out mat3 TBN;

struct DrawData {
    uint transformId;
    uint materialId;
};

layout(buffer_reference, std430) readonly buffer TransformBuffer {
    mat4 model[];
};

layout(buffer_reference, std430) readonly buffer DrawDataBuffer {
    DrawData dd[];
};

struct MaterialData {
    vec4 baseColorFactor;
    vec4 metallicRoughnessNormalOcclusion;
    vec4 emissiveFactorAlphaCutoff;
    uint baseColorTexture;
    uint normalTexture;
    uint metallicRoughnessTexture;
    uint emissiveTexture;
    uint materialflags; // Added flags field
    uint flags;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer {
    MaterialData material[];
};

struct TransparentFragment {
    f16vec4 color;
    float depth;
    uint next;
};

layout(buffer_reference, std430) buffer TransparencyListsBuffer {
    TransparentFragment frags[];
};

layout(std430, buffer_reference) buffer AtomicCounter {
    uint numFragments;
};

layout(std430, buffer_reference) buffer OIT {
    AtomicCounter atomicCounter;
    TransparencyListsBuffer oitLists;
    uint texHeadsOIT;
    uint maxOITFragments;
};

struct GPULight {
    vec3 position;
    float range;
    vec3 color;
    uint type;
    vec3 direction;
    float spotAngle;
};

struct ClusterBounds {
    vec3 minBounds;
    float pad0;
    vec3 maxBounds;
    float pad1;
};

struct LightList {
    uint offset;
    uint count;
};

layout(buffer_reference, std430) readonly buffer LightBuffer {
    GPULight lights[];
};

layout(buffer_reference, std430) readonly buffer ClusterBoundsBuffer {
    ClusterBounds bounds[];
};

layout(buffer_reference, std430) buffer LightIndicesBuffer {
    uint counter;        // Index 0: atomic counter
    uint indices[];      // Index 1+: actual light indices
};

layout(buffer_reference, std430) readonly buffer LightListsBuffer {
    LightList lists[];
};

// CORRECTED: Lighting buffer structure with proper buffer references
layout(std430, buffer_reference) buffer LightingData {
    LightBuffer lights;
    ClusterBoundsBuffer clusterBounds;  // Proper buffer reference, not uint64_t
    LightIndicesBuffer lightIndices;
    LightListsBuffer lightLists;
    uint totalLightCount;
    uint clusterDimX, clusterDimY, clusterDimZ;
    float zNear, zFar;
    vec2 screenDims;
    uint pad0, pad1;
};

layout(push_constant) uniform PerFrameData {
    mat4 viewProj;
    vec4 cameraPos;
    TransformBuffer transforms;
    DrawDataBuffer drawData;
    MaterialBuffer materials;
    OIT oit;
    LightingData lighting;
} pc;

void main() {
    DrawData dd = pc.drawData.dd[gl_BaseInstance];
    mat4 modelMatrix = pc.transforms.model[dd.transformId];
    
    vec4 worldPosition = modelMatrix * vec4(in_position, 1.0);
    gl_Position = pc.viewProj * worldPosition;
    
    uv = vec2(in_uv_x, in_uv_y);
    worldPos = worldPosition.xyz;
    materialId = dd.materialId;
    
    // Calculate TBN matrix using pre-computed mikktspace tangents
    mat3 normalMatrix = transpose(inverse(mat3(modelMatrix)));
    vec3 worldNormal = normalize(normalMatrix * in_normal);
    vec3 worldTangent = normalize(normalMatrix * in_tangent.xyz);
    vec3 worldBitangent = normalize(cross(worldNormal, worldTangent) * in_tangent.w);
    
    TBN = mat3(worldTangent, worldBitangent, worldNormal);
    normal = worldNormal; // Also pass simple normal for fallback
}