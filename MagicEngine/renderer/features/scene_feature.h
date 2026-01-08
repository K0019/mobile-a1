#pragma once

#include "renderer/interface.h"
#include "renderer/render_feature.h"
#include "renderer/render_graph.h"
#include "renderer/scene.h"
#include "math/utils_math.h"
#include <mutex>

namespace Resource
{
  class ResourceManager;
  struct Scene;
}

class Renderer;

struct DrawIndexedIndirectCommand
{
  uint32_t count;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t baseVertex;
  uint32_t baseInstance;
};

struct DrawData
{
  uint32_t transformId; // Index into transform arrays
  uint32_t materialId;  // Material index for GPU buffer access
  uint32_t meshDecompIndex; // Index into MeshDecompressionBuffer
  uint32_t objectId; // Index into scene's object array for identification
};
static_assert(sizeof(DrawData) == 16, "DrawData must be 16 bytes");

struct CullingData
{
  vec4 frustumPlanes[6];
  vec4 frustumCorners[8];
  uint32_t numMeshesToCull = 0;
  uint32_t numVisibleMeshes = 0;
};

struct PushConstants {
    uint64_t frameConstants;
    uint64_t bufferTransforms;
    uint64_t bufferDrawData;
    uint64_t bufferMaterials;
    uint64_t meshDecomp;
    uint64_t oitBuffer;
    uint64_t lightingBuffer;
    uint64_t textureIndices; // Reserved for future use
};

namespace Lighting
{
    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 8;
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t TOTAL_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_ITEMS_PER_CLUSTER = 256;
    static constexpr uint32_t MAX_TOTAL_ITEMS = 65536;

    // Point light shadow mapping constants
    static constexpr uint32_t MAX_SHADOW_POINT_LIGHTS = 4;
    static constexpr uint32_t SHADOW_MAP_SIZE = 1024;

    // GPU light structure - 32 bytes aligned
    struct GPULight
    {
        vec3 position; // World space position
        float range; // Light range (converted from attenuation)
        vec3 color; // RGB color * intensity
        uint32_t type; // LightType enum value
        vec3 direction; // Normalized direction (spot/directional)
        float spotAngle; // Cos of outer cone angle for spots
        // Total: 32 bytes
    };

    // Per-shadow-casting point light data for GPU - 416 bytes
    struct GPUPointLightShadow
    {
        mat4 viewProj[6];      // View-projection matrices for each cube face (384 bytes)
        vec4 lightPos;         // xyz = position, w = unused (16 bytes)
        float shadowNear;      // Near plane for shadow projection
        float shadowFar;       // Far plane for shadow projection  
        uint32_t shadowMapIndex; // Bindless cube texture index
        uint32_t lightIndex;   // Index into GPULight array
    };
    static_assert(sizeof(GPUPointLightShadow) == 416, "GPUPointLightShadow must be 416 bytes");

    // 3D cluster bounds in view space
    struct ClusterBounds
    {
        vec3 minBounds;
        float pad0;
        vec3 maxBounds;
        float pad1;
    };

    // Per-cluster item list entry
    struct Cluster
    {
        uint32_t offset; // Start index in the global item list buffer
        uint32_t count; // Number of items affecting this cluster
    };

    // Packed lighting buffer structure (following OIT pattern)
    struct LightingBuffer
    {
        uint64_t bufferLights; // GPULight array buffer address
        uint64_t bufferClusterBounds; // ClusterBounds array buffer address
        uint64_t bufferItemList; // Unified item list buffer address
        uint64_t bufferClusters; // Per-cluster offsets/counts
        uint64_t bufferPointShadows; // GPUPointLightShadow array buffer address
        vec2 screenDims; // Screen dimensions
        float zNear; // Camera near/far planes
        float zFar;
        uint32_t totalLightCount; // Number of active lights
        uint32_t shadowPointLightCount; // Number of shadow-casting point lights
        uint32_t pad0 = 0, pad1 = 0; // Align view matrix
        mat4 viewMatrix;
    };

    constexpr const char* LIGHT_BUFFER = "LightBuffer";
    constexpr const char* CLUSTER_BOUNDS = "ClusterBounds";
    constexpr const char* ITEM_LIST = "ItemList";
    constexpr const char* CLUSTER_DATA = "ClusterData";
    constexpr const char* LIGHTING_BUFFER = "Lighting_Buffer";
    constexpr const char* POINT_SHADOW_BUFFER = "PointShadowBuffer";
} // namespace Lighting

struct SceneRenderParams
{
  static constexpr uint32_t MAX_OBJECTS = 1000;
  static constexpr uint32_t MAX_LIGHTS = 1024; 

  // Object data
  std::vector<mat4> objectTransforms;
  std::vector<BoundingBox> objectBounds;
  std::vector<uint32_t> materialIndices;

  // Draw commands (all objects)
  std::vector<DrawIndexedIndirectCommand> drawCommands;
  std::vector<DrawData> drawData;

  // Render queues (indices into drawCommands array)
  std::vector<uint32_t> opaqueIndices;
  std::vector<uint32_t> transparentIndices;
  uint32_t opaqueStaticCount = 0;
  uint32_t opaqueAnimatedCount = 0;
  uint32_t transparentStaticCount = 0;
  uint32_t transparentAnimatedCount = 0;


  struct AnimatedInstanceParams
  {
    static constexpr uint32_t INVALID = 0xFFFFFFFFu;

    uint32_t drawIndex = INVALID;
    uint32_t srcBaseVertex = 0;         // Vertex offset in bind pose buffer
    uint32_t dstBaseVertex = 0;         // Vertex offset in skinned buffer
    uint32_t vertexCount = 0;
    uint32_t meshDecompIndex = 0;

    uint32_t skinningOffset = INVALID;  // GPUSkinningData element offset
    uint32_t boneMatrixOffset = INVALID;// mat4 offset into bone matrix buffer
    uint32_t jointCount = 0;

    uint32_t morphDeltaOffset = INVALID;// GPUMorphDelta element offset
    uint32_t morphDeltaCount = 0;
    uint32_t morphStateOffset = INVALID;// MorphRuntimeState offset
    uint32_t morphTargetCount = 0;
    uint32_t morphVertexBaseOffset = INVALID;  // uint32_t element offset
    uint32_t morphVertexCountOffset = INVALID; // uint32_t element offset
  };

  std::vector<mat4> boneMatrices;
  std::vector<float> morphWeights;
  std::vector<AnimatedInstanceParams> animatedInstances;
  std::vector<uint8_t> drawIsAnimated;
  uint32_t usedAnimatedVertexBytes = 0;
  bool hasAnimatedInstances = false;

  std::vector<Lighting::GPULight> lights;
  uint32_t activeLightCount = 0;

  // Point light shadow data
  std::vector<Lighting::GPUPointLightShadow> pointLightShadows;
  std::vector<uint32_t> shadowPointLightIndices; // Which lights cast shadows
  uint32_t shadowPointLightCount = 0;

  uint32_t irradianceTexture;
  uint32_t prefilterTexture;
  uint32_t brdfLUT;
  float environmentIntensity;

  uint32_t getObjectCount() const
  {
    return static_cast<uint32_t>(objectTransforms.size());
  }

  uint32_t getDrawCommandCount() const
  {
    return static_cast<uint32_t>(drawCommands.size());
  }

  uint32_t getOpaqueCount() const
  {
    return static_cast<uint32_t>(opaqueIndices.size());
  }

  uint32_t getTransparentCount() const
  {
    return static_cast<uint32_t>(transparentIndices.size());
  }

  uint32_t getLightCount() const { return activeLightCount; }

  void clear()
  {
    objectTransforms.clear();
    objectBounds.clear();
    materialIndices.clear();
    drawCommands.clear();
    drawData.clear();
    opaqueIndices.clear();
    transparentIndices.clear();
    opaqueStaticCount = 0;
    opaqueAnimatedCount = 0;
    transparentStaticCount = 0;
    transparentAnimatedCount = 0;
    lights.clear();
    activeLightCount = 0;
    pointLightShadows.clear();
    shadowPointLightIndices.clear();
    shadowPointLightCount = 0;
    boneMatrices.clear();
    morphWeights.clear();
    animatedInstances.clear();
    drawIsAnimated.clear();
    usedAnimatedVertexBytes = 0;
    hasAnimatedInstances = false;
  }

  void reserve(size_t objectCount)
  {
    objectTransforms.reserve(objectCount);
    objectBounds.reserve(objectCount);
    materialIndices.reserve(objectCount);
    drawCommands.reserve(objectCount);
    drawData.reserve(objectCount);
    opaqueIndices.reserve(objectCount);
    transparentIndices.reserve(objectCount);
    animatedInstances.reserve(objectCount);
    drawIsAnimated.reserve(objectCount);
    lights.reserve(std::min(static_cast<size_t>(MAX_LIGHTS), objectCount / 4)); //estimate
  }
};

class SceneRenderFeature final : public RenderFeatureBase<SceneRenderParams>
{
  using Parameters = SceneRenderParams;

  public:
    explicit SceneRenderFeature(bool enableObjectPicking = false);

    void static UpdateScene(uint64_t renderFeatureID,
                            Resource::Scene& gameScene,
                            const Resource::ResourceManager& asset_system,
                            Renderer& renderer);

    void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

    const char* GetName() const override;

    // Object picking API
    struct PickResult
    {
        bool valid = false;
        uint32_t sceneObjectIndex = 0;
        uint32_t primitiveId = 0;
        uint32_t drawId = 0;
    };

    void RequestObjectPick(int screenX, int screenY);
    PickResult GetLastPickResult() const;
    void ClearPickResult();

  private:

    static void convertSceneLight(const SceneLight& sceneLight, Lighting::GPULight& gpuLight);

    void ExecuteDeformationPass(internal::ExecutionContext& ctx);
    void ExecuteCullingPass(internal::ExecutionContext& ctx);

    void ExecuteOpaquePass(internal::ExecutionContext& ctx);

    void ExecuteTransparentPass(internal::ExecutionContext& ctx);

    void ExecutePointShadowPass(internal::ExecutionContext& ctx, uint32_t shadowLightIndex);

    void EnsurePointShadowPipeline(internal::ExecutionContext& ctx);

    void executeLightingSetup(internal::ExecutionContext& ctx);

    void executeClusterBoundsGeneration(internal::ExecutionContext& ctx);

    void executeLightCulling(internal::ExecutionContext& ctx);

    void EnsureLightingPipelines(internal::ExecutionContext& ctx);

    void ExecuteSkyboxPass(internal::ExecutionContext& ctx);

    void EnsureSkyboxPipeline(internal::ExecutionContext& ctx);

    void EnsureComputePipeline(internal::ExecutionContext& ctx);

    void EnsureRenderPipelines(internal::ExecutionContext& ctx);
    void EnsureDeformationPipeline(internal::ExecutionContext& ctx);

    uint32_t sampleCount = 1;

    vk::Holder<vk::ShaderModuleHandle> m_cullingShader;
    vk::Holder<vk::ComputePipelineHandle> m_deformationPipeline;
    vk::Holder<vk::ComputePipelineHandle> m_cullingPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_deformationShader;

    vk::Holder<vk::ShaderModuleHandle> vertShader;
    vk::Holder<vk::ShaderModuleHandle> m_skinnedVertShader;
    vk::Holder<vk::ShaderModuleHandle> opaquefragShader;
    vk::Holder<vk::ShaderModuleHandle> transparentfragShader;
    vk::Holder<vk::RenderPipelineHandle> m_opaqueRenderPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_opaqueAnimatedRenderPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_transparentRenderPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_transparentAnimatedRenderPipeline;

    vk::Holder<vk::RenderPipelineHandle> m_skyboxRenderPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_skyboxVertShader;
    vk::Holder<vk::ShaderModuleHandle> m_skyboxFragShader;

    vk::Holder<vk::ShaderModuleHandle> m_clusterBoundsShader;
    vk::Holder<vk::ComputePipelineHandle> m_clusterBoundsPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_lightCullingShader;
    vk::Holder<vk::ComputePipelineHandle> m_lightCullingPipeline;

    vk::Holder<vk::RenderPipelineHandle> m_depthPrepassPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_depthPrepassAnimatedPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_depthPrepassVertShader;
    vk::Holder<vk::ShaderModuleHandle> m_depthPrepassSkinnedVertShader;
    vk::Holder<vk::ShaderModuleHandle> m_depthPrepassFragShader;

    // Point light shadow mapping
    vk::Holder<vk::ShaderModuleHandle> m_pointShadowVertShader;
    vk::Holder<vk::ShaderModuleHandle> m_pointShadowFragShader;
    vk::Holder<vk::RenderPipelineHandle> m_pointShadowPipeline;
    
    // Point light shadow mapping (skinned/animated)
    vk::Holder<vk::ShaderModuleHandle> m_pointShadowSkinnedVertShader;
    vk::Holder<vk::RenderPipelineHandle> m_pointShadowSkinnedPipeline;

    void EnsureDepthPrepassPipeline(internal::ExecutionContext& ctx);
    void ExecuteDepthPrepass(internal::ExecutionContext& ctx);

    // Picking state
    bool m_enableObjectPicking = false;

    struct PickRequest
    {
        int screenX = 0;
        int screenY = 0;
        bool pending = false;
    };

    PickRequest m_pickRequest;
    PickResult m_lastPickResult;
    mutable std::mutex m_pickResultMutex;

    void ProcessPendingPick(internal::ExecutionContext& ctx);
};
