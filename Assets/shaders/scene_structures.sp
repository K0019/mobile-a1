// ============================================================================
// Scene Data Structures
// Shared between vertex and fragment shaders (no texture sampling functions)
// ============================================================================

struct DrawData {
    uint transformId;
    uint materialId;  // Material index (bits 0-30) + skinning flag (bit 31)
    uint meshDecompIndex;
    uint objectId;
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
    uint occlusionTexture;
    uint materialflags;
    uint flags;
};

layout(buffer_reference, std430) readonly buffer MaterialBuffer {
    MaterialData material[];
};

struct FrameConstantsGPU {
    mat4 viewProj;
    vec4 cameraPos;
    vec2 screenDims;
    float zNear;
    float zFar;
};

layout(buffer_reference, std140) readonly buffer FrameConstantsBuffer {
    FrameConstantsGPU constants;
};

// ============================================================================
// Animation System Data Structures
// ============================================================================

// Compressed vertex format (16 bytes) - matches C++ CompressedVertex
// Represented as 4 uints for compatibility (4 * 4 bytes = 16 bytes)
// Layout: [pos_xyz_nx] [packed] [uv_xy] [padding]
// - uint[0]: (pos_x:16, pos_y:16, pos_z:16, normal_x:16) as packed shorts
// - uint[1]: packed RGB10A2 (normal.y, tangent.xy, signs)
// - uint[2]: (uv_x:16, uv_y:16) as packed ushorts
// - uint[3]: unused/padding (for 16-byte alignment)
struct CompressedVertex {
    uvec4 data;  // Packed representation
};

layout(buffer_reference, std430) readonly buffer CompressedVertexBuffer {
    CompressedVertex vertices[];
};

struct SkinnedVertex {
    vec4 position;
    uint uvPacked;
    uint normalPacked;
    uint tangentPacked;
    uint padding;
};

layout(buffer_reference, std430) buffer SkinnedVertexBuffer {
    SkinnedVertex vertices[];
};

// Per-vertex skinning data (32 bytes) - matches C++ SkinningData
struct SkinningData {
    uvec4 boneIDs;      // Bone indices (up to 4 bones)
    vec4 boneWeights;   // Bone weights (must sum to 1.0)
};

layout(buffer_reference, std430) readonly buffer SkinningDataBuffer {
    SkinningData skins[];
};

// Morph target delta (48 bytes) - matches C++ MorphDelta
struct MorphDelta {
    uint morphTargetIndex;   // Target slot this delta contributes to
    uint vertexIndex;        // Original vertex index (unused at runtime)
    uint _pad0;
    uint _pad1;
    vec4 deltaPosition;      // xyz = delta position, w unused
    vec4 deltaNormal;        // xyz = delta normal,  w unused
    vec4 deltaTangent;       // xyz = delta tangent, w unused
};

layout(buffer_reference, std430) readonly buffer MorphDeltaBuffer {
    MorphDelta deltas[];
};

struct AnimatedInstance {
    uvec4 baseInfo;     // srcBaseVertex, dstBaseVertex, vertexCount, meshDecompIndex
    uvec4 skinningInfo; // skinningOffset, boneMatrixOffset, jointCount, morphStateOffset
    uvec4 morphInfo;    // morphDeltaOffset, morphVertexBaseOffset, morphVertexCountOffset, packed counts
};

layout(buffer_reference, std430) readonly buffer AnimatedInstanceBuffer {
    AnimatedInstance instances[];
};

layout(buffer_reference, std430) readonly buffer MorphWeightsBuffer {
    float weights[];
};

// Per-mesh morph state (64 bytes) - matches C++ MorphState
#define MAX_MORPHS_PER_MESH 8

struct MorphState {
    uint delta_base_indices[MAX_MORPHS_PER_MESH];  // Indices into MorphDelta buffer
    float weights[MAX_MORPHS_PER_MESH];             // Blend weights
};

layout(buffer_reference, std430) readonly buffer MorphStateBuffer {
    MorphState states[];
};

// Parallel buffers (one entry per vertex)
layout(buffer_reference, std430) readonly buffer VertexToMorphStateBuffer {
    uint indices[];  // Maps vertex index -> MorphState index (~0u = no morph)
};

layout(buffer_reference, std430) readonly buffer VertexLocalIndexBuffer {
    uint indices[];  // Local vertex index within mesh (0, 1, 2, ...)
};

// Bone matrices buffer
layout(buffer_reference, std430) readonly buffer BoneMatricesBuffer {
    mat4 matrices[];
};

// Legacy skeletal animation buffers (may be deprecated)
layout(buffer_reference, std430) readonly buffer SkinnedPositionBuffer {
    vec3 positions[];
};

layout(buffer_reference, std430) readonly buffer SkinnedNormalBuffer {
    vec3 normals[];
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

// Frustum-locked clustered lighting configuration
#define CLUSTER_DIM_X 16
#define CLUSTER_DIM_Y 8
#define CLUSTER_DIM_Z 24
#define MAX_ITEMS_PER_CLUSTER 256
#define MAX_SHADOW_POINT_LIGHTS 4

struct GPULight {
    vec3 position;
    float range;
    vec3 color;
    uint type;
    vec3 direction;
    float spotAngle;
};

// Per-shadow-casting point light data - 416 bytes
struct GPUPointLightShadow {
    mat4 viewProj[6];      // View-projection matrices for each cube face (384 bytes)
    vec4 lightPos;         // xyz = position, w = unused (16 bytes)
    float shadowNear;      // Near plane for shadow projection
    float shadowFar;       // Far plane for shadow projection
    uint shadowMapIndex;   // Bindless cube texture index
    uint lightIndex;       // Index into GPULight array
};

layout(buffer_reference, std430) readonly buffer PointLightShadowBuffer {
    GPUPointLightShadow shadows[];
};

struct ClusterBounds {
    vec3 minBounds;
    float pad0;
    vec3 maxBounds;
    float pad1;
};

struct Cluster {
    uint offset;
    uint count;
};

layout(buffer_reference, std430) readonly buffer LightBuffer {
    GPULight lights[];
};

layout(buffer_reference, std430) readonly buffer ClusterBoundsBuffer {
    ClusterBounds bounds[];
};

layout(buffer_reference, std430) buffer ItemListBuffer {
    uint counter;
    uint items[];
};

layout(buffer_reference, std430) buffer ClusterDataBuffer {
    Cluster clusters[];
};

layout(std430, buffer_reference) buffer LightingData {
    LightBuffer lights;
    ClusterBoundsBuffer clusterBounds;
    ItemListBuffer itemList;
    ClusterDataBuffer clusters;
    PointLightShadowBuffer pointShadows;  // Point light shadow data
    vec2 screenDims;
    float zNear;
    float zFar;
    uint totalLightCount;
    uint shadowPointLightCount;           // Number of shadow-casting point lights
    uint pad0, pad1;
    mat4 viewMatrix;
};
