#pragma once

#include "graphics/interface.h"
#include "graphics/render_feature.h"
#include "graphics/render_graph.h"
#include "graphics/scene.h"
#include "math/utils_math.h"

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
};

struct CullingData
{
  vec4 frustumPlanes[6];
  vec4 frustumCorners[8];
  uint32_t numMeshesToCull = 0;
  uint32_t numVisibleMeshes = 0;
};

struct PushConstants {
    mat4 viewProj;
    vec4 cameraPos;
    uint64_t bufferTransforms;
    uint64_t bufferDrawData;  
    uint64_t bufferMaterials;
    uint64_t oitBuffer;
    uint64_t lightingBuffer;  // NEW: Clustered lighting data
    uint32_t irradianceTexture;
    uint32_t prefilteredTexture;
    uint32_t brdfLutTexture;
    float environmentIntensity; // Padding to align to 16 bytes
};

namespace Lighting
{
    static constexpr uint32_t CLUSTER_DIM_X = 16;
    static constexpr uint32_t CLUSTER_DIM_Y = 9; 
    static constexpr uint32_t CLUSTER_DIM_Z = 24;
    static constexpr uint32_t TOTAL_CLUSTERS = CLUSTER_DIM_X * CLUSTER_DIM_Y * CLUSTER_DIM_Z;
    static constexpr uint32_t MAX_LIGHTS_PER_CLUSTER = 32; // Reduced from 1024
    
    // Reasonable buffer sizing
    static constexpr uint32_t MAX_TOTAL_LIGHT_INDICES = 16384;
  // GPU light structure - 32 bytes aligned
  struct GPULight
  {
    vec3 position;      // World space position
    float range;        // Light range (converted from attenuation)
    vec3 color;         // RGB color * intensity  
    uint32_t type;      // LightType enum value
    vec3 direction;     // Normalized direction (spot/directional)
    float spotAngle;    // Cos of outer cone angle for spots
    // Total: 32 bytes
  };

  // 3D cluster bounds in view space
  struct ClusterBounds
  {
    vec3 minBounds;
    float pad0;
    vec3 maxBounds; 
    float pad1;
  };

  // Per-cluster light list
  struct LightList
  {
    uint32_t offset;    // Start index in light indices buffer
    uint32_t count;     // Number of lights affecting this cluster
  };

  // Packed lighting buffer structure (following OIT pattern)
  struct LightingBuffer
  {
    uint64_t bufferLights;           // GPULight array buffer address
    uint64_t bufferClusterBounds;    // ClusterBounds array buffer address  
    uint64_t bufferLightIndices;     // Light indices buffer address
    uint64_t bufferLightLists;       // LightList array buffer address
    uint32_t totalLightCount;        // Number of active lights
    uint32_t clusterDimX;            // Cluster grid dimensions
    uint32_t clusterDimY;
    uint32_t clusterDimZ;
    float zNear;                     // Camera near/far planes
    float zFar;
    vec2 screenDims;                 // Screen dimensions
    uint32_t pad0, pad1;             // Align to 64 bytes
    mat4 viewMatrix;
  };

  constexpr const char* LIGHT_BUFFER = "LightBuffer";
  constexpr const char* CLUSTER_BOUNDS = "ClusterBounds";
  constexpr const char* LIGHT_INDICES = "LightIndices";
  constexpr const char* LIGHT_LISTS = "LightLists";
  constexpr const char* LIGHTING_BUFFER = "Lighting_Buffer";
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

  std::vector<Lighting::GPULight> lights;
  uint32_t activeLightCount = 0;

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
    lights.clear();
    activeLightCount = 0;
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
    lights.reserve(std::min(static_cast<size_t>(MAX_LIGHTS), objectCount / 4)); //estimate
  }
};

class SceneRenderFeature final : public RenderFeatureBase<SceneRenderParams>
{
  using Parameters = SceneRenderParams;

  public:
    void static UpdateScene(uint64_t renderFeatureID,
                            const Resource::Scene& gameScene,
                            const Resource::ResourceManager& asset_system,
                            Renderer& renderer);

    void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

    const char* GetName() const override;

  private:

    static void convertSceneLight(const SceneLight& sceneLight, Lighting::GPULight& gpuLight);

    void ExecuteCullingPass(internal::ExecutionContext& ctx);

    void ExecuteOpaquePass(internal::ExecutionContext& ctx);

    void ExecuteTransparentPass(internal::ExecutionContext& ctx);

    void executeLightingSetup(internal::ExecutionContext& ctx);

    void executeClusterBoundsGeneration(internal::ExecutionContext& ctx);

    void executeLightCulling(internal::ExecutionContext& ctx);

    void EnsureLightingPipelines(internal::ExecutionContext& ctx);

    void ExecuteSkyboxPass(internal::ExecutionContext& ctx);

    void EnsureSkyboxPipeline(internal::ExecutionContext& ctx);

    void EnsureComputePipeline(internal::ExecutionContext& ctx);

    void EnsureRenderPipelines(internal::ExecutionContext& ctx);

    uint32_t sampleCount = 1;

    vk::Holder<vk::ShaderModuleHandle> m_cullingShader;
    vk::Holder<vk::ComputePipelineHandle> m_cullingPipeline;

    vk::Holder<vk::ShaderModuleHandle> vertShader;
    vk::Holder<vk::ShaderModuleHandle> opaquefragShader;
    vk::Holder<vk::ShaderModuleHandle> transparentfragShader;
    vk::Holder<vk::RenderPipelineHandle> m_opaqueRenderPipeline;
    vk::Holder<vk::RenderPipelineHandle> m_transparentRenderPipeline;

    vk::Holder<vk::RenderPipelineHandle> m_skyboxRenderPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_skyboxVertShader;
    vk::Holder<vk::ShaderModuleHandle> m_skyboxFragShader;

    vk::Holder<vk::ShaderModuleHandle> m_clusterBoundsShader;
    vk::Holder<vk::ComputePipelineHandle> m_clusterBoundsPipeline;
    vk::Holder<vk::ShaderModuleHandle> m_lightCullingShader;
    vk::Holder<vk::ComputePipelineHandle> m_lightCullingPipeline;
};
