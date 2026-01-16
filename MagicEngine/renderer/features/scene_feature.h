#pragma once

#include "renderer/gfx_interface.h"
#include "renderer/gfx_types.h"
#include "renderer/render_feature.h"
#include "renderer/render_graph.h"
#include "renderer/scene.h"
#include "math/utils_math.h"
#include <mutex>
#include <vector>

// Forward declarations
class GfxRenderer;

namespace Resource
{
  class ResourceManager;
  class ResourceRegistry;
  struct Scene;
}

class Renderer;

// ============================================================================
// GPU Data Structures
// ============================================================================

struct DrawIndexedIndirectCommand
{
  uint32_t count;
  uint32_t instanceCount;
  uint32_t firstIndex;
  int32_t baseVertex;
  uint32_t baseInstance;
};

// Old GPU-driven draw data (kept for reference)
struct DrawDataGPU
{
  uint32_t transformId;
  uint32_t materialId;
  uint32_t meshDecompIndex;
  uint32_t objectId;
};
static_assert(sizeof(DrawDataGPU) == 16, "DrawDataGPU must be 16 bytes");

// CPU-driven draw data for immediate rendering
struct DrawData
{
  gfx::MeshHandle gfxMesh;
  gfx::MaterialHandle gfxMaterial;
  glm::mat4 transform;
  glm::vec4 baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
  bool hasMaterial = false;
};

struct CullingData
{
  vec4 frustumPlanes[6];
  vec4 frustumCorners[8];
  uint32_t numMeshesToCull = 0;
  uint32_t numVisibleMeshes = 0;
};

// ============================================================================
// GPU Light Structures - for deferred lighting pass
// Uses flat vec4 arrays for reliable std140 alignment (no nested structs)
// ============================================================================

static constexpr uint32_t MAX_DIRECTIONAL_LIGHTS = 4;
static constexpr uint32_t MAX_POINT_LIGHTS = 16;
static constexpr uint32_t MAX_SPOT_LIGHTS = 8;

// GPU Light UBO layout (std140) - flat vec4 arrays for reliable alignment
// Total size: 16 + 16 + (4*32) + (16*48) + (8*64) = 1432 bytes
struct GpuLightUBO
{
  // Header (32 bytes)
  glm::vec4 ambientColor;    // rgb = ambient, a = unused
  glm::uvec4 lightCounts;    // x = numDir, y = numPoint, z = numSpot, w = unused

  // Directional lights: 2 vec4s each (32 bytes per light)
  // [i*2+0]: direction.xyz, intensity
  // [i*2+1]: color.rgb, unused
  glm::vec4 dirLights[MAX_DIRECTIONAL_LIGHTS * 2];  // 128 bytes

  // Point lights: 3 vec4s each (48 bytes per light)
  // [i*3+0]: position.xyz, radius
  // [i*3+1]: color.rgb, intensity
  // [i*3+2]: attenuation.xyz, unused
  glm::vec4 pointLights[MAX_POINT_LIGHTS * 3];  // 768 bytes

  // Spot lights: 4 vec4s each (64 bytes per light)
  // [i*4+0]: position.xyz, radius
  // [i*4+1]: direction.xyz, cos(innerAngle)
  // [i*4+2]: color.rgb, intensity
  // [i*4+3]: attenuation.xyz, cos(outerAngle)
  glm::vec4 spotLights[MAX_SPOT_LIGHTS * 4];  // 512 bytes
};

// CPU-side collected light data (from ECS iteration)
struct CollectedLight
{
  LightType type;
  glm::vec3 position;
  glm::vec3 direction;
  glm::vec3 color;
  glm::vec3 attenuation;
  float intensity;
  float innerConeAngle;
  float outerConeAngle;
  float radius;  // Computed from attenuation for culling
};

// ============================================================================
// Scene Render Parameters - MINIMAL (SceneRenderFeature disabled)
// ============================================================================

struct SceneRenderParams
{
  // Camera/view parameters (still used by other systems)
  mat4 viewProj = mat4(1.0f);
  mat4 view = mat4(1.0f);
  mat4 proj = mat4(1.0f);
  vec3 cameraPosition = vec3(0.0f);
  float nearPlane = 0.1f;
  float farPlane = 1000.0f;

  // Feature toggles
  bool enableSSAO = true;
  bool enableShadows = true;
  bool enableWBOIT = true;
  bool enableSkybox = true;

  // Post-processing
  float exposure = 1.0f;
  float gamma = 2.2f;
};

// ============================================================================
// SceneRenderFeature - STUBBED (requires migration)
// ============================================================================

class SceneRenderFeature final : public RenderFeatureBase<SceneRenderParams>
{
  using Parameters = SceneRenderParams;

public:
  explicit SceneRenderFeature(bool enableObjectPicking = false);
  ~SceneRenderFeature();

  void Shutdown();

  /**
   * @brief Iterate ECS RenderComponents and submit meshes to GfxRenderer.
   *
   * This collects all renderable objects from the ECS and submits them
   * to GfxRenderer's draw queue for rendering in the G-buffer pass.
   *
   * @param renderFeatureID Feature handle for parameter access
   * @param resourceMngr ResourceManager for mesh/material lookups
   * @param renderer Main Renderer for access to GfxRenderer
   */
  static void UpdateScene(uint64_t renderFeatureID,
                          const Resource::ResourceManager& resourceMngr,
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
  // Pass implementations (all stubbed)
  void ExecuteSceneSetup(internal::ExecutionContext& ctx);
  void ExecuteDeformationPass(internal::ExecutionContext& ctx);
  void ExecuteLightingSetup(internal::ExecutionContext& ctx);
  void ExecuteGBufferPass(internal::ExecutionContext& ctx);
  void ExecuteDepthDownsample(internal::ExecutionContext& ctx);
  void ExecuteSSAO(internal::ExecutionContext& ctx);
  void ExecuteSSAOBlur(internal::ExecutionContext& ctx);
  void ExecuteShadowAtlasPass(internal::ExecutionContext& ctx);
  void ExecuteDeferredLighting(internal::ExecutionContext& ctx);
  void ExecuteWBOITAccumulation(internal::ExecutionContext& ctx);
  void ExecuteWBOITResolve(internal::ExecutionContext& ctx);
  void ExecuteSkyboxPass(internal::ExecutionContext& ctx);
  void ProcessPendingPick(internal::ExecutionContext& ctx);

  // Pipeline management (all stubbed)
  void EnsureDeformationPipeline(internal::ExecutionContext& ctx);
  void EnsureGBufferPipelines(internal::ExecutionContext& ctx);
  void EnsureSSAOPipelines(internal::ExecutionContext& ctx);
  void EnsureShadowPipelines(internal::ExecutionContext& ctx);
  void EnsureDeferredLightingPipeline(internal::ExecutionContext& ctx);
  void EnsureWBOITPipelines(internal::ExecutionContext& ctx);
  void EnsureSkyboxPipeline(internal::ExecutionContext& ctx);

  // ========================================================================
  // G-Buffer Pipeline Resources (owned by this feature)
  // ========================================================================
  gfx::Holder<gfx::Pipeline> m_gbufferPipeline;
  gfx::Holder<gfx::BindGroupLayout> m_sceneLayout;  // Set 0: Frame UBO
  gfx::Buffer m_frameUBO = {};                       // Persistently mapped frame UBO
  void* m_frameUBOMapped = nullptr;                  // Mapped pointer for fast updates
  gfx::BindGroup m_sceneBindGroup = {};              // Persistent bind group for frame data
  bool m_pipelineCreated = false;

  // ========================================================================
  // Composite Pipeline Resources (owned by this feature)
  // ========================================================================
  gfx::Holder<gfx::Pipeline> m_compositePipeline;
  gfx::Holder<gfx::BindGroupLayout> m_compositeLayout;  // Set 0: G-buffer textures
  gfx::Holder<gfx::Sampler> m_compositeSampler;
  gfx::Buffer m_fullscreenQuadVB = {};
  gfx::BindGroup m_compositeBindGroup = {};           // Persistent bind group for G-buffer
  bool m_compositePipelineCreated = false;
  uint32_t m_lastGBufferWidth = 0;                    // Track G-buffer size for rebind
  uint32_t m_lastGBufferHeight = 0;
  gfx::Texture m_lastSceneDepth = {};                 // Track depth texture for rebind

  // Per-frame draw list (populated in UpdateScene)
  std::vector<DrawData> m_drawList;

  // Per-frame light list (populated in UpdateScene)
  std::vector<CollectedLight> m_lightList;

  // ========================================================================
  // Light UBO Resources (for deferred lighting pass)
  // ========================================================================
  gfx::Holder<gfx::BindGroupLayout> m_lightLayout;  // Set 1: Light UBO
  gfx::BindGroup m_lightBindGroup = {};
  gfx::Buffer m_lightUBO = {};
  void* m_lightUBOMapped = nullptr;
  bool m_lightUBOCreated = false;

  // Lazy pipeline creation
  bool EnsurePipelineCreated(GfxRenderer* gfxRenderer);
  bool EnsureCompositePipelineCreated(GfxRenderer* gfxRenderer);
  bool EnsureCompositeBindGroup(GfxRenderer* gfxRenderer, gfx::Texture sceneDepth, gfx::TextureView sceneDepthView);
  void ExecuteCompositePass(internal::ExecutionContext& ctx);

  // State
  bool m_enableObjectPicking = false;
  uint32_t m_sampleCount = 1;

  struct PickRequest
  {
    int screenX = 0;
    int screenY = 0;
    bool pending = false;
  };

  PickRequest m_pickRequest;
  PickResult m_lastPickResult;
  mutable std::mutex m_pickResultMutex;
};
