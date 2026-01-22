// =============================================================================
// scene_feature.cpp - Scene G-Buffer Rendering Feature
//
// Owns G-buffer shader, pipeline, and rendering resources. Integrates with
// RenderGraph for uniform allocation and GfxRenderer for mesh/material storage.
// =============================================================================

#include "scene_feature.h"
#include "renderer/hina_context.h"
#include "renderer/gfx_renderer.h"
#include "renderer/gfx_mesh_storage.h"
#include "renderer/gfx_material_system.h"
#include "renderer/render_graph.h"
#include "renderer/linear_color.h"
#include "resource/resource_manager.h"
#include "resource/resource_types.h"
#include "Graphics/RenderComponent.h"
#include "Graphics/LightComponent.h"
#include "Graphics/AnimationComponent.h"
#include "ECS/ECS.h"
#include "VFS/VFS.h"
#include "logging/log.h"
#include "math/utils_math.h"  // For BoundingBox (frustum culling)
#include <cstring>
#include <cmath>
#include <cassert>

// =============================================================================
// Shader Loading Helper
// =============================================================================

static bool LoadShaderSource(const char* vfsPath, std::string& outSource)
{
  if (!VFS::ReadFile(vfsPath, outSource)) {
    LOG_ERROR("[SceneRenderFeature] Failed to load shader: {}", vfsPath);
    return false;
  }
  return true;
}

// Shader file paths (relative to VFS mount point, which maps "" -> "./Assets")
static const char* kGBufferShaderPath = "shaders/gbuffer.hina_sl";
static const char* kGBufferSkinnedShaderPath = "shaders/gbuffer_skinned.hina_sl";
static const char* kCompositeShaderPath = "shaders/composite.hina_sl";
static const char* kWBOITAccumShaderPath = "shaders/wboit_accum.hina_sl";
static const char* kWBOITResolveShaderPath = "shaders/wboit_resolve.hina_sl";


// =============================================================================
// SceneRenderFeature Implementation
// =============================================================================

SceneRenderFeature::SceneRenderFeature(bool enableObjectPicking)
  : m_enableObjectPicking(enableObjectPicking)
{
}

SceneRenderFeature::~SceneRenderFeature()
{
  Shutdown();
}

void SceneRenderFeature::Shutdown()
{
  // G-buffer resources
  if (hina_bind_group_is_valid(m_sceneBindGroup)) {
    hina_destroy_bind_group(m_sceneBindGroup);
    m_sceneBindGroup = {};
  }
  if (hina_buffer_is_valid(m_frameUBO)) {
    // Note: HOST_VISIBLE buffers are persistently mapped, don't need to unmap
    hina_destroy_buffer(m_frameUBO);
    m_frameUBO = {};
  }
  m_frameUBOMapped = nullptr;
  m_gbufferPipeline.reset();
  m_gbufferSkinnedPipeline.reset();
  m_sceneLayout.reset();
  m_pipelineCreated = false;
  m_skinnedPipelineCreated = false;

  // Composite resources
  if (hina_bind_group_is_valid(m_compositeBindGroup)) {
    hina_destroy_bind_group(m_compositeBindGroup);
    m_compositeBindGroup = {};
  }
  m_compositePipeline.reset();
  m_compositeLayout.reset();
  m_compositeSampler.reset();
  if (hina_buffer_is_valid(m_fullscreenQuadVB)) {
    hina_destroy_buffer(m_fullscreenQuadVB);
    m_fullscreenQuadVB = {};
  }
  m_compositePipelineCreated = false;
  m_lastGBufferWidth = 0;
  m_lastGBufferHeight = 0;

  // Light resources
  if (hina_bind_group_is_valid(m_lightBindGroup)) {
    hina_destroy_bind_group(m_lightBindGroup);
    m_lightBindGroup = {};
  }
  m_lightLayout.reset();
  if (hina_buffer_is_valid(m_lightUBO)) {
    hina_destroy_buffer(m_lightUBO);
    m_lightUBO = {};
  }
  m_lightUBOMapped = nullptr;
  m_lightUBOCreated = false;

  // WBOIT resources
  if (hina_bind_group_is_valid(m_wboitFrameBindGroup)) {
    hina_destroy_bind_group(m_wboitFrameBindGroup);
    m_wboitFrameBindGroup = {};
  }
  if (hina_bind_group_is_valid(m_wboitResolveBindGroup)) {
    hina_destroy_bind_group(m_wboitResolveBindGroup);
    m_wboitResolveBindGroup = {};
  }
  if (hina_buffer_is_valid(m_wboitFrameUBO)) {
    hina_destroy_buffer(m_wboitFrameUBO);
    m_wboitFrameUBO = {};
  }
  m_wboitFrameUBOMapped = nullptr;
  m_wboitAccumPipeline.reset();
  m_wboitResolvePipeline.reset();
  m_wboitFrameLayout.reset();
  m_wboitResolveLayout.reset();
  m_wboitSampler.reset();
  m_wboitPipelinesCreated = false;
  m_lastWboitAccum = {};

  m_drawList.clear();
  m_transparentDrawList.clear();
  m_lightList.clear();
}

bool SceneRenderFeature::EnsurePipelineCreated(GfxRenderer* gfxRenderer)
{
  if (m_pipelineCreated) {
    return true;
  }

  if (!gfxRenderer) {
    return false;
  }

  // Pipeline creation (one-time)

  // Load shader source from file
  std::string shaderSource;
  if (!LoadShaderSource(kGBufferShaderPath, shaderSource)) {
    return false;
  }

  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(shaderSource.c_str(), "scene_gbuffer", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // Verify we got valid SPIR-V data
  if (module->vs.spirv_size == 0 || module->fs.spirv_size == 0) {
    LOG_ERROR("[SceneRenderFeature] Module has zero-size SPIR-V data!");
    hslc_hsl_module_free(module);
    return false;
  }
  if (!module->vs.spirv_data || !module->fs.spirv_data) {
    LOG_ERROR("[SceneRenderFeature] Module has NULL SPIR-V data pointers!");
    hslc_hsl_module_free(module);
    return false;
  }

  // Scene bind group layout (set 0: frame UBO)
  hina_bind_group_layout_entry scene_entries[1] = {};
  scene_entries[0].binding = 0;
  scene_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  scene_entries[0].count = 1;
  scene_entries[0].stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
  scene_entries[0].flags = HINA_BINDING_FLAG_NONE;

  hina_bind_group_layout_desc scene_layout_desc = {};
  scene_layout_desc.entries = scene_entries;
  scene_layout_desc.entry_count = 1;
  scene_layout_desc.label = "scene_feature_frame_layout";

  m_sceneLayout.reset(hina_create_bind_group_layout(&scene_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_sceneLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create scene bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // Material layout from GfxRenderer (Set 1: constants UBO + textures)
  gfx::BindGroupLayout materialLayout = gfxRenderer->getMaterialLayout();
  // Dynamic layout from GfxRenderer (Set 2: transform UBO with dynamic offset)
  gfx::BindGroupLayout dynamicLayout = gfxRenderer->getDynamicLayout();

  // Vertex layout for split streams
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 2;
  vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition);
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes);
  vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.attr_count = 4;
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };
  vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };
  vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };
  vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;  // Disable culling - meshes have inconsistent winding
  pip_desc.front_face = HINA_FRONT_FACE_CLOCKWISE;  // CW = front (matches objects)
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = true;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pip_desc.color_attachment_count = 3;
  pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;
  pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;
  pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;
  // Set 0: Frame UBO, Set 1: Material (constants UBO + textures), Set 2: Dynamic transform UBO
  pip_desc.bind_group_layouts[0] = m_sceneLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  pip_desc.bind_group_layout_count = 3;

  m_gbufferPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_gbufferPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Pipeline creation failed");
    return false;
  }

  // Create persistent frame UBO (viewProj matrix = 64 bytes)
  hina_buffer_desc ubo_desc = {};
  ubo_desc.size = sizeof(glm::mat4);
  ubo_desc.flags = static_cast<hina_buffer_flags>(
      HINA_BUFFER_UNIFORM_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
  m_frameUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_frameUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create frame UBO");
    return false;
  }

  // Persistently map the UBO for fast per-frame updates
  m_frameUBOMapped = hina_map_buffer(m_frameUBO);
  if (!m_frameUBOMapped) {
    LOG_ERROR("[SceneRenderFeature] Failed to map frame UBO");
    return false;
  }

  // Create persistent bind group pointing to the frame UBO
  hina_bind_group_entry scene_entry = {};
  scene_entry.binding = 0;
  scene_entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  scene_entry.buffer.buffer = m_frameUBO;
  scene_entry.buffer.offset = 0;
  scene_entry.buffer.size = sizeof(glm::mat4);

  hina_bind_group_desc scene_bg_desc = {};
  scene_bg_desc.layout = m_sceneLayout.get();
  scene_bg_desc.entries = &scene_entry;
  scene_bg_desc.entry_count = 1;
  scene_bg_desc.label = "scene_frame_bg";

  m_sceneBindGroup = hina_create_bind_group(&scene_bg_desc);
  if (!hina_bind_group_is_valid(m_sceneBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create scene bind group");
    return false;
  }

  m_pipelineCreated = true;
  return true;
}

bool SceneRenderFeature::EnsureSkinnedPipelineCreated(GfxRenderer* gfxRenderer)
{
  if (m_skinnedPipelineCreated) {
    return true;
  }

  // Static pipeline must be created first (we reuse some resources)
  if (!m_pipelineCreated) {
    if (!EnsurePipelineCreated(gfxRenderer)) {
      return false;
    }
  }

  // Load shader source from file
  std::string shaderSource;
  if (!LoadShaderSource(kGBufferSkinnedShaderPath, shaderSource)) {
    return false;
  }

  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(shaderSource.c_str(), "scene_gbuffer_skinned", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Skinned shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // Material layout from GfxRenderer (Set 1: constants UBO + textures)
  gfx::BindGroupLayout materialLayout = gfxRenderer->getMaterialLayout();
  // Dynamic layout from GfxRenderer (Set 2: transform UBO + bone matrices with dynamic offsets)
  gfx::BindGroupLayout dynamicLayout = gfxRenderer->getDynamicLayout();

  // Vertex layout for split streams (3 buffers for skinned meshes)
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 3;
  // Buffer 0: Position stream (12 bytes)
  vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition);
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 1: Attribute stream (12 bytes)
  vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes);
  vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 2: Skinning stream (8 bytes)
  vertex_layout.buffer_strides[2] = sizeof(gfx::VertexSkinning);
  vertex_layout.input_rates[2] = HINA_VERTEX_INPUT_RATE_VERTEX;

  // 6 attributes total
  vertex_layout.attr_count = 6;
  // Buffer 0 attributes
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };  // position
  // Buffer 1 attributes
  vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };   // normalX
  vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };   // packed
  vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };   // uv
  // Buffer 2 attributes (skinning)
  vertex_layout.attrs[4] = { HINA_FORMAT_R8G8B8A8_UINT, 0, 4, 2 };    // boneIndices
  vertex_layout.attrs[5] = { HINA_FORMAT_R8G8B8A8_UNORM, 4, 5, 2 };   // boneWeights

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.front_face = HINA_FRONT_FACE_CLOCKWISE;
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = true;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pip_desc.color_attachment_count = 3;
  pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;
  pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;
  pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;
  pip_desc.bind_group_layouts[0] = m_sceneLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  pip_desc.bind_group_layout_count = 3;

  m_gbufferSkinnedPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_gbufferSkinnedPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Skinned pipeline creation failed");
    return false;
  }

  m_skinnedPipelineCreated = true;
  return true;
}

void SceneRenderFeature::UpdateScene(uint64_t renderFeatureID,
                                      const Resource::ResourceManager& resourceMngr,
                                      GfxRenderer& renderer)
{
  auto* feature = renderer.GetFeature<SceneRenderFeature>(renderFeatureID);
  if (!feature) {
    return;
  }

  feature->m_drawList.clear();
  feature->m_transparentDrawList.clear();

  for (auto compIter = ecs::GetCompsActiveBegin<RenderComponent>();
       compIter != ecs::GetCompsEnd<RenderComponent>();
       ++compIter)
  {
    RenderComponent& comp = *compIter;

    const ResourceMesh* mesh = comp.GetMesh();
    if (!mesh) continue;

    auto* entity = ecs::GetEntity(&comp);
    if (!entity) continue;

    glm::mat4 entityWorldMatrix;
    entity->GetTransform().SetMat4ToWorld(&entityWorldMatrix);

    // Check for AnimationComponent for skeletal skinning
    // Use skinMatrices.size() as authoritative count (AnimationComponent may resize it)
    AnimationComponent* animComp = entity->GetComp<AnimationComponent>();
    bool hasValidSkinning = animComp && !animComp->skinMatrices.empty();

    const auto& materialList = comp.GetMaterialsList();
    const size_t materialCount = materialList.size();

    for (size_t i = 0; i < mesh->handles.size(); ++i)
    {
      const MeshHandle& meshHandle = mesh->handles[i];
      if (!meshHandle.isValid()) continue;

      const MeshGPUData* meshData = resourceMngr.getMesh(meshHandle);
      if (!meshData || !meshData->hasGfxMesh) continue;

      // Frustum culling - skip objects outside camera view
      const gfx::GpuMesh* gpuMesh = renderer.getMeshStorage().get(
          gfx::MeshHandle{static_cast<uint16_t>(meshData->gfxMeshIndex),
                          static_cast<uint16_t>(meshData->gfxMeshGeneration)});
      if (gpuMesh && gpuMesh->valid) {
        // Transform AABB to world space
        glm::mat4 worldTransform = entityWorldMatrix * mesh->transforms[i];
        BoundingBox worldBounds(gpuMesh->bounds.aabbMin, gpuMesh->bounds.aabbMax);
        worldBounds.transform(worldTransform);

        // Inflate bounds for skinned meshes (1.5x) to accommodate animations
        if (hasValidSkinning) {
          vec3 center = worldBounds.getCenter();
          vec3 halfSize = worldBounds.getSize() * 0.5f * 1.5f;
          worldBounds.min_ = center - halfSize;
          worldBounds.max_ = center + halfSize;
        }

        // Skip if not visible in frustum
        if (!renderer.isVisibleInFrustum(worldBounds.min_, worldBounds.max_)) {
          continue;
        }
      }

      DrawData draw;
      draw.gfxMesh.index = static_cast<uint16_t>(meshData->gfxMeshIndex);
      draw.gfxMesh.generation = static_cast<uint16_t>(meshData->gfxMeshGeneration);
      draw.transform = entityWorldMatrix * mesh->transforms[i];
      draw.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

      // Set skinning data if entity has valid animation
      if (hasValidSkinning) {
        draw.isSkinned = true;
        // Use skinMatrices.size() as authoritative - AnimationComponent::ProcessComp may resize it
        draw.jointCount = static_cast<uint32_t>(animComp->skinMatrices.size());
        draw.skinMatrices = animComp->skinMatrices.data();
      }

      const ResourceMaterial* material = nullptr;
      if (i < materialCount) {
        material = materialList[i].GetResource();
      }
      if (!material) {
        material = MagicResourceManager::GetContainer<ResourceMaterial>().GetResource(
            mesh->defaultMaterialHashes[i]);
      }

      // Check material and determine if transparent
      bool isTransparent = false;
      if (material && material->handle.isValid()) {
        const auto* matHotData = resourceMngr.getMaterialHotData(material->handle);
        if (matHotData && matHotData->hasGfxMaterial) {
          draw.gfxMaterial.index = static_cast<uint16_t>(matHotData->gfxMaterialIndex);
          draw.gfxMaterial.generation = static_cast<uint16_t>(matHotData->gfxMaterialGeneration);
          draw.hasMaterial = true;

          // Check render queue for transparency routing
          if (matHotData->renderQueue == RenderQueue::Transparent) {
            isTransparent = true;
          }
        }
      }

      // Route draw to appropriate list based on transparency
      draw.isTransparent = isTransparent;
      if (isTransparent) {
        feature->m_transparentDrawList.push_back(draw);
      } else {
        feature->m_drawList.push_back(draw);
      }
    }
  }

  // =========================================================================
  // Collect lights from ECS LightComponents
  // =========================================================================
  feature->m_lightList.clear();

  for (auto compIter = ecs::GetCompsActiveBegin<LightComponent>();
       compIter != ecs::GetCompsEnd<LightComponent>();
       ++compIter)
  {
    LightComponent& lightComp = *compIter;

    auto* entity = ecs::GetEntity(&lightComp);
    if (!entity) continue;

    const SceneLight& sceneLight = lightComp.light;

    // Get world position and direction from entity transform
    glm::mat4 entityWorldMatrix;
    entity->GetTransform().SetMat4ToWorld(&entityWorldMatrix);

    CollectedLight collected;
    collected.type = sceneLight.type;
    collected.position = glm::vec3(entityWorldMatrix[3]);  // Extract position from matrix
    collected.direction = glm::normalize(glm::vec3(entityWorldMatrix * glm::vec4(sceneLight.direction, 0.0f)));
    collected.color = sceneLight.color;
    collected.attenuation = sceneLight.attenuation;
    collected.intensity = sceneLight.intensity;
    collected.innerConeAngle = sceneLight.innerConeAngle;
    collected.outerConeAngle = sceneLight.outerConeAngle;

    // Compute radius for culling (where attenuation reaches ~1% intensity)
    // Using quadratic attenuation: 1/(c + l*d + q*d^2) = 0.01
    // Simplified: radius = sqrt(100/quadratic) when quadratic > 0
    float quadratic = sceneLight.attenuation.z;
    if (quadratic > 0.001f) {
      collected.radius = std::sqrt(100.0f / quadratic);
    } else {
      collected.radius = 100.0f;  // Very large fallback
    }

    feature->m_lightList.push_back(collected);
  }

}

void SceneRenderFeature::SetupPasses(internal::RenderPassBuilder& passBuilder)
{
  // G-buffer pass - renders to G-buffer color textures (GfxRenderer) and SCENE_DEPTH (RenderGraph)
  // SCENE_DEPTH is shared with Grid/Im3d/UI2D for depth testing
  passBuilder.CreatePass()
    .SetPriority(internal::RenderPassBuilder::PassPriority::Opaque)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Write)
    .AddGenericPass("SceneGBuffer", [this](internal::ExecutionContext& ctx) {
      ExecuteGBufferPass(ctx);
    });

  // Composite pass - renders deferred lighting to SCENE_COLOR (HDR)
  // This is a proper RenderGraph pass that outputs to SCENE_COLOR
  // NOTE: No depth attachment needed - composite is fullscreen quad, doesn't depth-test
  PassDeclarationInfo compositePassInfo;
  compositePassInfo.framebufferDebugName = "SceneComposite";
  compositePassInfo.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR,
    .loadOp = gfx::LoadOp::Clear,
    .storeOp = gfx::StoreOp::Store,
    .clearColor = {0.1f, 0.1f, 0.15f, 1.0f}  // Sky color as clear
  };
  // No depth attachment - composite pipeline samples G-buffer depth as texture, doesn't test depth

  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Read)  // Reads depth as texture, not attachment
    .SetPriority(internal::RenderPassBuilder::PassPriority::PostProcess)
    .ExecuteAfter("SceneGBuffer")
    .AddGraphicsPass("SceneComposite", compositePassInfo, [this](const internal::ExecutionContext& ctx) {
      // Const cast needed because ExecuteCompositePass modifies internal state
      ExecuteCompositePass(const_cast<internal::ExecutionContext&>(ctx));
    });

  // =========================================================================
  // WBOIT Passes (Order-Independent Transparency)
  // =========================================================================
  // WBOIT Accumulation pass - renders transparent objects with weighted blending
  // Uses SCENE_DEPTH for depth testing (read-only), writes to WBOIT_Accumulation (owned by GfxRenderer)
  // Priority 510: Must run AFTER SceneComposite (500) which writes the opaque scene to SCENE_COLOR
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Read)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(510))
    .ExecuteAfter("SceneComposite")
    .AddGenericPass("WBOITAccumulation", [this](internal::ExecutionContext& ctx) {
      ExecuteWBOITAccumulation(ctx);
    });

  // WBOIT Resolve pass - composites transparent result over SCENE_COLOR
  // WBOIT accumulation texture is accessed directly from GfxRenderer (external to render graph)
  // Priority 520: Must run AFTER WBOITAccumulation (510)
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(520))
    .ExecuteAfter("WBOITAccumulation")
    .AddGenericPass("WBOITResolve", [this](internal::ExecutionContext& ctx) {
      ExecuteWBOITResolve(ctx);
    });
}

void SceneRenderFeature::ExecuteGBufferPass(internal::ExecutionContext& ctx)
{
  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) return;

  // Ensure pipeline and persistent resources are created
  if (!EnsurePipelineCreated(gfxRenderer)) return;

  // Assert camera matrices are valid - identity means camera wasn't set up
  const FrameData& frameData = ctx.GetFrameData();
  assert(frameData.projMatrix != glm::mat4(1.0f) && "projMatrix is identity! Camera projection not set.");
  assert(frameData.viewMatrix != glm::mat4(1.0f) && "viewMatrix is identity! Camera view not set.");

  gfx::Cmd* cmd = ctx.GetCmd();

  // G-buffer color targets from GfxRenderer
  const auto& gbuffer = gfxRenderer->getGBuffer();

  // Use SCENE_DEPTH from RenderGraph (shared with Grid/Im3d/UI2D)
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Always clear the G-buffer to transition textures out of UNDEFINED layout
  // This ensures the composite pass can sample them even if nothing is drawn
  hina_pass_action pass = {};
  pass.colors[0].image = gbuffer.albedoView;
  pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
  pass.colors[0].store_op = HINA_STORE_OP_STORE;

  pass.colors[1].image = gbuffer.normalView;
  pass.colors[1].load_op = HINA_LOAD_OP_CLEAR;
  pass.colors[1].store_op = HINA_STORE_OP_STORE;
  pass.colors[1].clear_color[0] = 0.5f;
  pass.colors[1].clear_color[1] = 0.5f;

  pass.colors[2].image = gbuffer.materialDataView;
  pass.colors[2].load_op = HINA_LOAD_OP_CLEAR;
  pass.colors[2].store_op = HINA_STORE_OP_STORE;

  // Use SCENE_DEPTH (RenderGraph transient) instead of gbuffer.depthView
  // This way Grid/Im3d/UI2D can use the same depth for overlay rendering
  pass.depth.image = sceneDepthView;
  pass.depth.load_op = HINA_LOAD_OP_CLEAR;
  pass.depth.store_op = HINA_STORE_OP_STORE;
  pass.depth.depth_clear = 1.0f;

  hina_cmd_begin_pass(cmd, &pass);

  // Update frame UBO via persistently mapped pointer (HOST_COHERENT, no flush needed)
  // Use camera matrices from FrameData (populated by ECS camera system), NOT from SceneRenderParams
  // Note: frameData already retrieved at function start for assert checks

  // projMatrix already has Vulkan Y-flip applied at source (UploadToPipeline)
  glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  std::memcpy(m_frameUBOMapped, &viewProj, sizeof(glm::mat4));

  // Bind pipeline and persistent frame bind group
  hina_cmd_bind_pipeline(cmd, m_gbufferPipeline.get());
  hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

  // Draw submitted meshes
  if (!m_drawList.empty()) {
    auto& meshStorage = gfxRenderer->getMeshStorage();
    auto& materialSystem = gfxRenderer->getMaterialSystem();

    // Get frame resources for transform and bone matrix ring buffers
    auto& frame = gfxRenderer->getCurrentFrameResources();

    // Use persistently mapped pointers (mapped once during creation)
    if (!frame.transformRingMapped) {
      hina_cmd_end_pass(cmd);
      return;
    }

    // Separate draws into static and skinned
    std::vector<const DrawData*> staticDraws;
    std::vector<const DrawData*> skinnedDraws;
    staticDraws.reserve(m_drawList.size());
    skinnedDraws.reserve(32);  // Typical skinned draw count

    for (const auto& draw : m_drawList) {
      // Only classify as skinned if mesh also has skinning data
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      bool meshHasSkinning = gpuMesh && gpuMesh->valid && gpuMesh->hasSkinning;
      if (draw.isSkinned && draw.skinMatrices && meshHasSkinning) {
        skinnedDraws.push_back(&draw);
      } else {
        staticDraws.push_back(&draw);
      }
    }

    // =========================================================================
    // Pass 1: Render static meshes with m_gbufferPipeline
    // =========================================================================
    for (const DrawData* drawPtr : staticDraws) {
      const DrawData& draw = *drawPtr;
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      if (!gpuMesh || !gpuMesh->valid) continue;

      // Check transform ring buffer overflow
      if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
        break;
      }

      // Write transform to ring buffer at current offset
      glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(
          static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset);
      *transformDst = draw.transform;

      // Bind split vertex streams with offsets (2 buffers for static)
      hina_vertex_input meshInput = {};
      meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
      meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
      meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
      meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
      meshInput.index_buffer = gpuMesh->indexBuffer;
      meshInput.index_type = HINA_INDEX_UINT32;

      hina_cmd_apply_vertex_input(cmd, &meshInput);

      // Bind material at Set 1
      gfx::BindGroup materialBG;
      if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
        materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
      } else {
        materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
      }
      hina_cmd_bind_group(cmd, 1, materialBG);

      // Bind Set 2 with 2 dynamic offsets (transform + dummy bone offset)
      uint32_t dynamicOffsets[2] = { frame.transformRingOffset, 0 };
      hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);

      // Advance transform ring offset
      frame.transformRingOffset += TRANSFORM_ALIGNMENT;

      // Draw
      hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
    }

    // =========================================================================
    // Pass 2: Render skinned meshes with m_gbufferSkinnedPipeline
    // =========================================================================
    if (!skinnedDraws.empty()) {
      // Ensure skinned pipeline is created
      if (!EnsureSkinnedPipelineCreated(gfxRenderer) || !frame.boneMatrixRingMapped) {
        // Skip skinned draws if pipeline or bone buffer not available
      } else {
        // Switch to skinned pipeline
        hina_cmd_bind_pipeline(cmd, m_gbufferSkinnedPipeline.get());
        hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

        for (const DrawData* drawPtr : skinnedDraws) {
          const DrawData& draw = *drawPtr;
          const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
          if (!gpuMesh || !gpuMesh->valid) continue;

          // Check buffer overflow
          if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE ||
              frame.boneMatrixRingOffset >= BONE_MATRIX_UBO_SIZE) {
            break;
          }

          // Write transform to ring buffer
          glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(
              static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset);
          *transformDst = draw.transform;

          // Write bone matrices to ring buffer (up to MAX_BONES_PER_MESH)
          // Initialize all slots to identity first, then copy actual bones
          uint32_t numBones = std::min(draw.jointCount, gfx::MAX_BONES_PER_MESH);
          if (draw.jointCount > gfx::MAX_BONES_PER_MESH) {
            static bool warnedOnce = false;
            if (!warnedOnce) {
              LOG_WARNING("[SceneRenderFeature] Mesh has {} bones but MAX_BONES_PER_MESH is {}. "
                          "Bones beyond limit will use identity matrices. Consider bone remapping.",
                          draw.jointCount, gfx::MAX_BONES_PER_MESH);
              warnedOnce = true;
            }
          }
          glm::mat4* bonesDst = reinterpret_cast<glm::mat4*>(
              static_cast<uint8_t*>(frame.boneMatrixRingMapped) + frame.boneMatrixRingOffset);

          // Fill all slots with identity matrices first (prevents garbage in unused slots)
          static const glm::mat4 identity(1.0f);
          for (uint32_t i = 0; i < gfx::MAX_BONES_PER_MESH; ++i) {
            bonesDst[i] = identity;
          }
          // Copy actual bone matrices
          std::memcpy(bonesDst, draw.skinMatrices, numBones * sizeof(glm::mat4));

          // Bind split vertex streams (3 buffers for skinned)
          hina_vertex_input meshInput = {};
          meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
          meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
          meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
          meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
          meshInput.vertex_buffers[2] = gpuMesh->skinningBuffer;
          meshInput.vertex_offsets[2] = gpuMesh->skinningOffset;
          meshInput.index_buffer = gpuMesh->indexBuffer;
          meshInput.index_type = HINA_INDEX_UINT32;

          hina_cmd_apply_vertex_input(cmd, &meshInput);

          // Bind material at Set 1
          gfx::BindGroup materialBG;
          if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
            materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
          } else {
            materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
          }
          hina_cmd_bind_group(cmd, 1, materialBG);

          // Bind Set 2 with 2 dynamic offsets (transform + bone matrices)
          uint32_t dynamicOffsets[2] = { frame.transformRingOffset, frame.boneMatrixRingOffset };
          hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);

          // Advance ring buffer offsets
          frame.transformRingOffset += TRANSFORM_ALIGNMENT;
          frame.boneMatrixRingOffset += gfx::BONE_MATRICES_SIZE;

          // Draw
          hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
        }
      }
    }

    // No unmap needed for HOST_COHERENT persistently mapped buffers
  }

  hina_cmd_end_pass(cmd);
}

const char* SceneRenderFeature::GetName() const
{
  return "SceneRenderFeature";
}

void SceneRenderFeature::RequestObjectPick(int screenX, int screenY)
{
  std::lock_guard<std::mutex> lock(m_pickResultMutex);
  m_pickRequest.screenX = screenX;
  m_pickRequest.screenY = screenY;
  m_pickRequest.pending = true;
}

SceneRenderFeature::PickResult SceneRenderFeature::GetLastPickResult() const
{
  std::lock_guard<std::mutex> lock(m_pickResultMutex);
  return m_lastPickResult;
}

void SceneRenderFeature::ClearPickResult()
{
  std::lock_guard<std::mutex> lock(m_pickResultMutex);
  m_lastPickResult = {};
}

// Stubbed pass implementations (to be implemented later)
void SceneRenderFeature::ExecuteSceneSetup(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteDeformationPass(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteDepthDownsample(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteSSAO(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteSSAOBlur(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteShadowAtlasPass(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteDeferredLighting(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteSkyboxPass(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteWBOITAccumulation(internal::ExecutionContext& ctx)
{
  // Skip if no transparent draws
  if (m_transparentDrawList.empty()) {
    return;
  }

  // Ensure pipelines are created (also creates light UBO if composite pipeline exists)
  EnsureWBOITPipelines(ctx);
  if (!m_wboitPipelinesCreated) {
    return;
  }

  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  const FrameData& frameData = ctx.GetFrameData();
  gfx::Cmd* cmd = ctx.GetCmd();
  const auto& meshStorage = gfxRenderer->getMeshStorage();
  const auto& materialSystem = gfxRenderer->getMaterialSystem();
  const auto& wboit = gfxRenderer->getWBOIT();
  auto& frame = gfxRenderer->getCurrentFrameResources();

  // Get scene depth for depth testing
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Begin WBOIT accumulation pass
  hina_pass_action pass = {};
  pass.colors[0].image = wboit.accumulationView;
  pass.colors[0].load_op = HINA_LOAD_OP_CLEAR;
  pass.colors[0].store_op = HINA_STORE_OP_STORE;
  pass.colors[0].clear_color[0] = 0.0f;
  pass.colors[0].clear_color[1] = 0.0f;
  pass.colors[0].clear_color[2] = 0.0f;
  pass.colors[0].clear_color[3] = 0.0f;

  pass.depth.image = sceneDepthView;
  pass.depth.load_op = HINA_LOAD_OP_LOAD;  // Preserve opaque depth
  pass.depth.store_op = HINA_STORE_OP_STORE;

  hina_cmd_begin_pass(cmd, &pass);

  // Update WBOIT frame UBO (viewProj + cameraPos)
  if (m_wboitFrameUBOMapped) {
    glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
    glm::vec4 cameraPos(frameData.cameraPos, 0.0f);

    std::memcpy(m_wboitFrameUBOMapped, &viewProj, sizeof(glm::mat4));
    std::memcpy(static_cast<uint8_t*>(m_wboitFrameUBOMapped) + sizeof(glm::mat4), &cameraPos, sizeof(glm::vec4));
  }

  uint32_t width = RenderResources::INTERNAL_WIDTH;
  uint32_t height = RenderResources::INTERNAL_HEIGHT;

  hina_viewport viewport = {};
  viewport.width = static_cast<float>(width);
  viewport.height = static_cast<float>(height);
  viewport.min_depth = 0.0f;
  viewport.max_depth = 1.0f;
  hina_cmd_set_viewport(cmd, &viewport);

  hina_scissor scissor = {};
  scissor.width = width;
  scissor.height = height;
  hina_cmd_set_scissor(cmd, &scissor);

  // Bind accumulation pipeline
  hina_cmd_bind_pipeline(cmd, m_wboitAccumPipeline.get());

  // Bind frame bind group (set 0)
  hina_cmd_bind_group(cmd, 0, m_wboitFrameBindGroup);

  // Bind light bind group (set 3) - reuse from composite
  if (hina_bind_group_is_valid(m_lightBindGroup)) {
    hina_cmd_bind_group(cmd, 3, m_lightBindGroup);
  }

  // Render each transparent draw
  for (const DrawData& draw : m_transparentDrawList) {
    const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
    if (!gpuMesh || !gpuMesh->valid) continue;

    // Check transform ring buffer overflow
    if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
      break;
    }

    // Write transform to ring buffer
    glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(
        static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset);
    *transformDst = draw.transform;

    // Bind vertex streams
    hina_vertex_input meshInput = {};
    meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
    meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
    meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
    meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
    meshInput.index_buffer = gpuMesh->indexBuffer;
    meshInput.index_type = HINA_INDEX_UINT32;

    hina_cmd_apply_vertex_input(cmd, &meshInput);

    // Bind material at Set 1
    gfx::BindGroup materialBG;
    if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
      materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
    } else {
      materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
    }
    hina_cmd_bind_group(cmd, 1, materialBG);

    // Bind Set 2 with dynamic offset
    uint32_t dynamicOffsets[2] = { frame.transformRingOffset, 0 };
    hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);

    // Advance transform ring offset
    frame.transformRingOffset += TRANSFORM_ALIGNMENT;

    // Draw
    hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
  }

  hina_cmd_end_pass(cmd);
}
void SceneRenderFeature::ExecuteWBOITResolve(internal::ExecutionContext& ctx)
{
  // Skip if no transparent draws or pipeline not created
  if (m_transparentDrawList.empty() || !m_wboitPipelinesCreated) {
    return;
  }

  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  gfx::Cmd* cmd = ctx.GetCmd();
  const auto& wboit = gfxRenderer->getWBOIT();

  // Get HDR scene color to composite over
  gfx::Texture sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  gfx::TextureView sceneColorView = hina_texture_get_default_view(sceneColor);

  // Ensure resolve bind group is created/updated
  bool needsRecreate = !hina_bind_group_is_valid(m_wboitResolveBindGroup)
                    || wboit.accumulation.id != m_lastWboitAccum.id;

  if (needsRecreate) {
    if (hina_bind_group_is_valid(m_wboitResolveBindGroup)) {
      hina_destroy_bind_group(m_wboitResolveBindGroup);
      m_wboitResolveBindGroup = {};
    }

    hina_bind_group_entry entry = {};
    entry.binding = 0;
    entry.type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entry.combined.view = wboit.accumulationView;
    entry.combined.sampler = m_wboitSampler.get();

    hina_bind_group_desc bg_desc = {};
    bg_desc.layout = m_wboitResolveLayout.get();
    bg_desc.entries = &entry;
    bg_desc.entry_count = 1;
    bg_desc.label = "wboit_resolve_bg";

    m_wboitResolveBindGroup = hina_create_bind_group(&bg_desc);
    m_lastWboitAccum = wboit.accumulation;
  }

  if (!hina_bind_group_is_valid(m_wboitResolveBindGroup)) {
    return;
  }

  // Begin resolve pass (composite over SCENE_COLOR)
  hina_pass_action pass = {};
  pass.colors[0].image = sceneColorView;
  pass.colors[0].load_op = HINA_LOAD_OP_LOAD;  // Preserve opaque scene
  pass.colors[0].store_op = HINA_STORE_OP_STORE;

  hina_cmd_begin_pass(cmd, &pass);

  uint32_t width = RenderResources::INTERNAL_WIDTH;
  uint32_t height = RenderResources::INTERNAL_HEIGHT;

  hina_viewport viewport = {};
  viewport.width = static_cast<float>(width);
  viewport.height = static_cast<float>(height);
  viewport.min_depth = 0.0f;
  viewport.max_depth = 1.0f;
  hina_cmd_set_viewport(cmd, &viewport);

  hina_scissor scissor = {};
  scissor.width = width;
  scissor.height = height;
  hina_cmd_set_scissor(cmd, &scissor);

  // Bind resolve pipeline
  hina_cmd_bind_pipeline(cmd, m_wboitResolvePipeline.get());

  // Bind accumulation texture
  hina_cmd_bind_group(cmd, 0, m_wboitResolveBindGroup);

  // Bind fullscreen quad (reuse from composite)
  hina_vertex_input vertInput = {};
  vertInput.vertex_buffers[0] = m_fullscreenQuadVB;
  vertInput.vertex_offsets[0] = 0;
  hina_cmd_apply_vertex_input(cmd, &vertInput);

  // Draw fullscreen quad
  hina_cmd_draw(cmd, 6, 1, 0, 0);

  hina_cmd_end_pass(cmd);
}
void SceneRenderFeature::ProcessPendingPick(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteLightingSetup(internal::ExecutionContext& ctx) { (void)ctx; }

void SceneRenderFeature::EnsureDeformationPipeline(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureGBufferPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureSSAOPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureShadowPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureDeferredLightingPipeline(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureWBOITPipelines(internal::ExecutionContext& ctx)
{
  if (m_wboitPipelinesCreated) {
    return;
  }

  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  // =========================================================================
  // Create WBOIT Accumulation Pipeline
  // =========================================================================

  // Load accumulation shader
  std::string accumShaderSource;
  if (!LoadShaderSource(kWBOITAccumShaderPath, accumShaderSource)) {
    LOG_ERROR("[SceneRenderFeature] Failed to load WBOIT accumulation shader");
    return;
  }

  char* error = nullptr;
  hina_hsl_module* accumModule = hslc_compile_hsl_source(accumShaderSource.c_str(), "wboit_accum", &error);
  if (!accumModule) {
    LOG_ERROR("[SceneRenderFeature] WBOIT accumulation shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return;
  }

  // Set 0: Frame UBO (viewProj + cameraPos)
  hina_bind_group_layout_entry frame_entries[1] = {};
  frame_entries[0].binding = 0;
  frame_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  frame_entries[0].count = 1;
  frame_entries[0].stage_flags = HINA_STAGE_VERTEX | HINA_STAGE_FRAGMENT;
  frame_entries[0].flags = HINA_BINDING_FLAG_NONE;

  hina_bind_group_layout_desc frame_layout_desc = {};
  frame_layout_desc.entries = frame_entries;
  frame_layout_desc.entry_count = 1;
  frame_layout_desc.label = "wboit_frame_layout";

  m_wboitFrameLayout.reset(hina_create_bind_group_layout(&frame_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_wboitFrameLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create WBOIT frame bind group layout");
    hslc_hsl_module_free(accumModule);
    return;
  }

  // Create Frame UBO (mat4 viewProj + vec4 cameraPos = 80 bytes)
  constexpr size_t kWBOITFrameUBOSize = sizeof(glm::mat4) + sizeof(glm::vec4);
  hina_buffer_desc ubo_desc = {};
  ubo_desc.size = kWBOITFrameUBOSize;
  ubo_desc.flags = static_cast<hina_buffer_flags>(
      HINA_BUFFER_UNIFORM_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
  m_wboitFrameUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_wboitFrameUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create WBOIT frame UBO");
    hslc_hsl_module_free(accumModule);
    return;
  }
  m_wboitFrameUBOMapped = hina_map_buffer(m_wboitFrameUBO);

  // Create frame bind group
  hina_bind_group_entry frame_bg_entry = {};
  frame_bg_entry.binding = 0;
  frame_bg_entry.type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  frame_bg_entry.buffer.buffer = m_wboitFrameUBO;
  frame_bg_entry.buffer.offset = 0;
  frame_bg_entry.buffer.size = kWBOITFrameUBOSize;

  hina_bind_group_desc frame_bg_desc = {};
  frame_bg_desc.layout = m_wboitFrameLayout.get();
  frame_bg_desc.entries = &frame_bg_entry;
  frame_bg_desc.entry_count = 1;
  frame_bg_desc.label = "wboit_frame_bg";

  m_wboitFrameBindGroup = hina_create_bind_group(&frame_bg_desc);
  if (!hina_bind_group_is_valid(m_wboitFrameBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create WBOIT frame bind group");
    hslc_hsl_module_free(accumModule);
    return;
  }

  // Get material and dynamic layouts from GfxRenderer
  gfx::BindGroupLayout materialLayout = gfxRenderer->getMaterialLayout();
  gfx::BindGroupLayout dynamicLayout = gfxRenderer->getDynamicLayout();

  // Vertex layout for split streams (same as gbuffer)
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 2;
  vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition);
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes);
  vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.attr_count = 4;
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };     // position
  vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };              // normalX
  vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };              // packed
  vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };              // uv

  // Pipeline descriptor with additive blending
  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.front_face = HINA_FRONT_FACE_CLOCKWISE;
  pip_desc.depth.depth_test = true;   // Test against opaque depth
  pip_desc.depth.depth_write = false; // Don't write depth for transparents
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pip_desc.color_attachment_count = 1;
  pip_desc.color_formats[0] = HINA_FORMAT_R16G16B16A16_SFLOAT;  // Accumulation (RGBA16F)

  // Additive blending: final = src * 1 + dst * 1
  pip_desc.blend.enable = true;
  pip_desc.blend.src_color = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend.dst_color = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
  pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
  pip_desc.blend.color_write_mask = HINA_COLOR_COMPONENT_ALL;

  // Set 0: Frame, Set 1: Material, Set 2: Dynamic, Set 3: Lights
  pip_desc.bind_group_layouts[0] = m_wboitFrameLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  pip_desc.bind_group_layouts[3] = m_lightLayout.get();  // Reuse from composite
  pip_desc.bind_group_layout_count = 4;

  m_wboitAccumPipeline.reset(hina_make_pipeline_from_module(accumModule, &pip_desc, nullptr));
  hslc_hsl_module_free(accumModule);

  if (!hina_pipeline_is_valid(m_wboitAccumPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT accumulation pipeline creation failed");
    return;
  }

  // =========================================================================
  // Create WBOIT Resolve Pipeline
  // =========================================================================

  std::string resolveShaderSource;
  if (!LoadShaderSource(kWBOITResolveShaderPath, resolveShaderSource)) {
    LOG_ERROR("[SceneRenderFeature] Failed to load WBOIT resolve shader");
    return;
  }

  hina_hsl_module* resolveModule = hslc_compile_hsl_source(resolveShaderSource.c_str(), "wboit_resolve", &error);
  if (!resolveModule) {
    LOG_ERROR("[SceneRenderFeature] WBOIT resolve shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return;
  }

  // Create sampler for resolve pass
  gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
  samplerDesc.min_filter = HINA_FILTER_LINEAR;
  samplerDesc.mag_filter = HINA_FILTER_LINEAR;
  samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
  samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
  m_wboitSampler.reset(gfx::createSampler(samplerDesc));

  // Set 0: Accumulation texture (combined image sampler)
  hina_bind_group_layout_entry resolve_entries[1] = {};
  resolve_entries[0].binding = 0;
  resolve_entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  resolve_entries[0].count = 1;
  resolve_entries[0].stage_flags = HINA_STAGE_FRAGMENT;
  resolve_entries[0].flags = HINA_BINDING_FLAG_NONE;

  hina_bind_group_layout_desc resolve_layout_desc = {};
  resolve_layout_desc.entries = resolve_entries;
  resolve_layout_desc.entry_count = 1;
  resolve_layout_desc.label = "wboit_resolve_layout";

  m_wboitResolveLayout.reset(hina_create_bind_group_layout(&resolve_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_wboitResolveLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create WBOIT resolve bind group layout");
    hslc_hsl_module_free(resolveModule);
    return;
  }

  // Fullscreen quad vertex layout
  hina_vertex_layout resolve_vertex_layout = {};
  resolve_vertex_layout.buffer_count = 1;
  resolve_vertex_layout.buffer_strides[0] = sizeof(float) * 4;
  resolve_vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  resolve_vertex_layout.attr_count = 2;
  resolve_vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, 0, 0, 0 };              // position
  resolve_vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, sizeof(float) * 2, 1, 0 }; // uv

  // Resolve pipeline with compositing blend
  // Blend: final = src * (1 - srcAlpha) + dst * srcAlpha
  // This composites transparent over opaque: src = transparent result, srcAlpha = reveal
  hina_hsl_pipeline_desc resolve_pip_desc = hina_hsl_pipeline_desc_default();
  resolve_pip_desc.layout = resolve_vertex_layout;
  resolve_pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  resolve_pip_desc.depth.depth_test = false;
  resolve_pip_desc.depth.depth_write = false;
  resolve_pip_desc.depth_format = HINA_FORMAT_UNDEFINED;
  resolve_pip_desc.color_attachment_count = 1;
  resolve_pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;

  // Compositing blend: final = avgColor * (1 - reveal) + opaque * reveal
  resolve_pip_desc.blend.enable = true;
  resolve_pip_desc.blend.src_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  resolve_pip_desc.blend.dst_color = HINA_BLEND_FACTOR_SRC_ALPHA;
  resolve_pip_desc.blend.color_op = HINA_BLEND_OP_ADD;
  resolve_pip_desc.blend.src_alpha = HINA_BLEND_FACTOR_ZERO;
  resolve_pip_desc.blend.dst_alpha = HINA_BLEND_FACTOR_ONE;
  resolve_pip_desc.blend.alpha_op = HINA_BLEND_OP_ADD;
  resolve_pip_desc.blend.color_write_mask = HINA_COLOR_COMPONENT_ALL;

  resolve_pip_desc.bind_group_layouts[0] = m_wboitResolveLayout.get();
  resolve_pip_desc.bind_group_layout_count = 1;

  m_wboitResolvePipeline.reset(hina_make_pipeline_from_module(resolveModule, &resolve_pip_desc, nullptr));
  hslc_hsl_module_free(resolveModule);

  if (!hina_pipeline_is_valid(m_wboitResolvePipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT resolve pipeline creation failed");
    return;
  }

  m_wboitPipelinesCreated = true;
  LOG_DEBUG("[SceneRenderFeature] WBOIT pipelines created successfully");
}
void SceneRenderFeature::EnsureSkyboxPipeline(internal::ExecutionContext& ctx) { (void)ctx; }

// =============================================================================
// Composite Pipeline Creation and Execution
// =============================================================================

bool SceneRenderFeature::EnsureCompositePipelineCreated(GfxRenderer* gfxRenderer)
{
  if (m_compositePipelineCreated) {
    return true;
  }

  if (!gfxRenderer) {
    return false;
  }

  // Create composite pipeline (one-time)

  // Create sampler for G-buffer sampling
  {
    gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
    samplerDesc.min_filter = HINA_FILTER_NEAREST;
    samplerDesc.mag_filter = HINA_FILTER_NEAREST;
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_compositeSampler.reset(gfx::createSampler(samplerDesc));
  }

  // Create fullscreen quad vertex buffer
  {
    const float quadVerts[] = {
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f, -1.0f, 1.0f, 0.0f,
       1.0f,  1.0f, 1.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 1.0f, 1.0f,
      -1.0f,  1.0f, 0.0f, 1.0f
    };

    hina_buffer_desc vb_desc = {};
    vb_desc.size = sizeof(quadVerts);
    vb_desc.flags = static_cast<hina_buffer_flags>(
        HINA_BUFFER_VERTEX_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
    m_fullscreenQuadVB = hina_make_buffer(&vb_desc);
    if (!hina_buffer_is_valid(m_fullscreenQuadVB)) {
      LOG_ERROR("[SceneRenderFeature] Failed to create fullscreen quad vertex buffer");
      return false;
    }
    void* mapped = hina_map_buffer(m_fullscreenQuadVB);
    std::memcpy(mapped, quadVerts, sizeof(quadVerts));
  }

  // Load and compile composite shader
  std::string shaderSource;
  if (!LoadShaderSource(kCompositeShaderPath, shaderSource)) {
    return false;
  }

  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(shaderSource.c_str(), "scene_composite", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Composite shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // =========================================================================
  // Set 0: G-buffer textures (4 combined image samplers)
  // =========================================================================
  hina_bind_group_layout_entry gbuffer_entries[4] = {};
  for (int i = 0; i < 4; ++i) {
    gbuffer_entries[i].binding = static_cast<uint32_t>(i);
    gbuffer_entries[i].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    gbuffer_entries[i].count = 1;
    gbuffer_entries[i].stage_flags = HINA_STAGE_FRAGMENT;
    gbuffer_entries[i].flags = HINA_BINDING_FLAG_NONE;
  }

  hina_bind_group_layout_desc gbuffer_layout_desc = {};
  gbuffer_layout_desc.entries = gbuffer_entries;
  gbuffer_layout_desc.entry_count = 4;
  gbuffer_layout_desc.label = "composite_gbuffer_layout";

  m_compositeLayout.reset(hina_create_bind_group_layout(&gbuffer_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_compositeLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create G-buffer bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // =========================================================================
  // Set 1: Light UBO
  // Layout: vec4 ambient, vec4 cameraPos, mat4 invViewProj, ivec4 lightCounts, vec4[128] lights
  // Total: 16 + 16 + 64 + 16 + 2048 = 2160 bytes
  // =========================================================================
  hina_bind_group_layout_entry light_entries[1] = {};
  light_entries[0].binding = 0;
  light_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  light_entries[0].count = 1;
  light_entries[0].stage_flags = HINA_STAGE_FRAGMENT;
  light_entries[0].flags = HINA_BINDING_FLAG_NONE;

  hina_bind_group_layout_desc light_layout_desc = {};
  light_layout_desc.entries = light_entries;
  light_layout_desc.entry_count = 1;
  light_layout_desc.label = "composite_light_layout";

  m_lightLayout.reset(hina_create_bind_group_layout(&light_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_lightLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create light bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // Create light UBO buffer (2160 bytes, host visible for per-frame updates)
  constexpr size_t kLightUBOSize = 16 + 16 + 64 + 16 + (128 * 16);  // 2160 bytes
  hina_buffer_desc ubo_desc = {};
  ubo_desc.size = kLightUBOSize;
  ubo_desc.flags = static_cast<hina_buffer_flags>(
      HINA_BUFFER_UNIFORM_BIT | HINA_BUFFER_HOST_VISIBLE_BIT | HINA_BUFFER_HOST_COHERENT_BIT);
  m_lightUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_lightUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create light UBO");
    hslc_hsl_module_free(module);
    return false;
  }
  m_lightUBOMapped = hina_map_buffer(m_lightUBO);

  // Create light bind group
  hina_bind_group_entry light_bg_entries[1] = {};
  light_bg_entries[0].binding = 0;
  light_bg_entries[0].type = HINA_DESC_TYPE_UNIFORM_BUFFER;
  light_bg_entries[0].buffer.buffer = m_lightUBO;
  light_bg_entries[0].buffer.offset = 0;
  light_bg_entries[0].buffer.size = kLightUBOSize;

  hina_bind_group_desc light_bg_desc = {};
  light_bg_desc.layout = m_lightLayout.get();
  light_bg_desc.entries = light_bg_entries;
  light_bg_desc.entry_count = 1;
  light_bg_desc.label = "composite_light_bg";

  m_lightBindGroup = hina_create_bind_group(&light_bg_desc);
  if (!hina_bind_group_is_valid(m_lightBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create light bind group");
    hslc_hsl_module_free(module);
    return false;
  }
  m_lightUBOCreated = true;

  // =========================================================================
  // Create pipeline with both bind group layouts
  // =========================================================================
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 1;
  vertex_layout.buffer_strides[0] = sizeof(float) * 4;
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.attr_count = 2;
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, 0, 0, 0 };  // position
  vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, sizeof(float) * 2, 1, 0 };  // uv

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.depth.depth_test = false;
  pip_desc.depth.depth_write = false;
  pip_desc.depth_format = HINA_FORMAT_UNDEFINED;
  pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;
  pip_desc.color_attachment_count = 1;
  pip_desc.bind_group_layouts[0] = m_compositeLayout.get();
  pip_desc.bind_group_layouts[1] = m_lightLayout.get();
  pip_desc.bind_group_layout_count = 2;

  m_compositePipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_compositePipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Composite pipeline creation failed");
    return false;
  }

  m_compositePipelineCreated = true;
  return true;
}

bool SceneRenderFeature::EnsureCompositeBindGroup(GfxRenderer* gfxRenderer, gfx::Texture sceneDepth, gfx::TextureView sceneDepthView)
{
  if (!gfxRenderer) return false;

  const auto& gbuffer = gfxRenderer->getGBuffer();

  // Check if G-buffer dimensions changed (resize occurred) or depth texture changed
  uint32_t gbufferWidth = 0, gbufferHeight = 0;
  hina_get_texture_size(gbuffer.albedo, &gbufferWidth, &gbufferHeight);

  bool needsRecreate = !hina_bind_group_is_valid(m_compositeBindGroup)
                    || gbufferWidth != m_lastGBufferWidth
                    || gbufferHeight != m_lastGBufferHeight
                    || sceneDepth.id != m_lastSceneDepth.id;

  if (!needsRecreate) {
    return true;  // Existing bind group is still valid
  }

  // Destroy old bind group if it exists
  if (hina_bind_group_is_valid(m_compositeBindGroup)) {
    hina_destroy_bind_group(m_compositeBindGroup);
    m_compositeBindGroup = {};
  }

  // Create new bind group with current G-buffer textures
  hina_bind_group_entry entries[4] = {};
  entries[0].binding = 0;  // albedo
  entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  entries[0].combined.view = gbuffer.albedoView;
  entries[0].combined.sampler = m_compositeSampler.get();

  entries[1].binding = 1;  // normal
  entries[1].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  entries[1].combined.view = gbuffer.normalView;
  entries[1].combined.sampler = m_compositeSampler.get();

  entries[2].binding = 2;  // materialData
  entries[2].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  entries[2].combined.view = gbuffer.materialDataView;
  entries[2].combined.sampler = m_compositeSampler.get();

  entries[3].binding = 3;  // depth (from SCENE_DEPTH)
  entries[3].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  entries[3].combined.view = sceneDepthView;
  entries[3].combined.sampler = m_compositeSampler.get();

  hina_bind_group_desc bg_desc = {};
  bg_desc.layout = m_compositeLayout.get();
  bg_desc.entries = entries;
  bg_desc.entry_count = 4;
  bg_desc.label = "scene_composite_bg";

  m_compositeBindGroup = hina_create_bind_group(&bg_desc);
  if (!hina_bind_group_is_valid(m_compositeBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create composite bind group");
    return false;
  }

  m_lastGBufferWidth = gbufferWidth;
  m_lastGBufferHeight = gbufferHeight;
  m_lastSceneDepth = sceneDepth;
  return true;
}

void SceneRenderFeature::ExecuteCompositePass(internal::ExecutionContext& ctx)
{
  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  if (!EnsureCompositePipelineCreated(gfxRenderer)) {
    return;
  }

  gfx::Cmd* cmd = ctx.GetCmd();
  const FrameData& frameData = ctx.GetFrameData();

  // Get SCENE_COLOR and SCENE_DEPTH from RenderGraph
  gfx::Texture sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  if (!gfx::isValid(sceneColor)) {
    return;
  }

  // Get depth view for sampling
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Ensure composite bind group is created/updated
  if (!EnsureCompositeBindGroup(gfxRenderer, sceneDepth, sceneDepthView)) {
    return;
  }

  // =========================================================================
  // Fill Light UBO with all collected lights
  // Layout (std140):
  //   vec4 ambientColor    (16 bytes, offset 0)
  //   vec4 cameraPos       (16 bytes, offset 16)
  //   mat4 invViewProj     (64 bytes, offset 32)
  //   ivec4 lightCounts    (16 bytes, offset 96)
  //   vec4 lights[128]     (2048 bytes, offset 112)
  // Each light uses 4 vec4s:
  //   [0]: type, position.xyz
  //   [1]: direction.xyz, intensity
  //   [2]: color.rgb, radius
  //   [3]: reserved (attenuation.xyz, unused)
  // =========================================================================
  if (m_lightUBOMapped) {
    uint8_t* uboData = static_cast<uint8_t*>(m_lightUBOMapped);
    size_t offset = 0;

    // ambientColor (offset 0)
    glm::vec4 ambient(0.15f, 0.17f, 0.2f, 1.0f);
    std::memcpy(uboData + offset, &ambient, sizeof(glm::vec4));
    offset += 16;

    // cameraPos (offset 16)
    glm::vec4 cameraPos(frameData.cameraPos, 1.0f);
    std::memcpy(uboData + offset, &cameraPos, sizeof(glm::vec4));
    offset += 16;

    // invViewProj (offset 32)
    glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
    glm::mat4 invViewProj = glm::inverse(viewProj);
    std::memcpy(uboData + offset, &invViewProj, sizeof(glm::mat4));
    offset += 64;

    // lightCounts (offset 96)
    int numLights = static_cast<int>(std::min(m_lightList.size(), size_t(32)));
    glm::ivec4 lightCounts(numLights, 0, 0, 0);
    std::memcpy(uboData + offset, &lightCounts, sizeof(glm::ivec4));
    offset += 16;

    // lights array (offset 112)
    // Each light is 4 vec4s = 64 bytes
    for (int i = 0; i < numLights; ++i) {
      const CollectedLight& light = m_lightList[i];

      // data0: type, position.xyz
      glm::vec4 data0(static_cast<float>(light.type), light.position.x, light.position.y, light.position.z);
      std::memcpy(uboData + offset, &data0, sizeof(glm::vec4));
      offset += 16;

      // data1: direction.xyz, intensity
      glm::vec4 data1(light.direction.x, light.direction.y, light.direction.z, light.intensity);
      std::memcpy(uboData + offset, &data1, sizeof(glm::vec4));
      offset += 16;

      // data2: color.rgb, radius
      glm::vec4 data2(light.color.x, light.color.y, light.color.z, light.radius);
      std::memcpy(uboData + offset, &data2, sizeof(glm::vec4));
      offset += 16;

      // data3: attenuation.xyz, unused (reserved for future use)
      glm::vec4 data3(light.attenuation.x, light.attenuation.y, light.attenuation.z, 0.0f);
      std::memcpy(uboData + offset, &data3, sizeof(glm::vec4));
      offset += 16;
    }
  }

  uint32_t width = RenderResources::INTERNAL_WIDTH;
  uint32_t height = RenderResources::INTERNAL_HEIGHT;

  // Set viewport and scissor
  hina_viewport viewport = {};
  viewport.width = static_cast<float>(width);
  viewport.height = static_cast<float>(height);
  viewport.min_depth = 0.0f;
  viewport.max_depth = 1.0f;
  hina_cmd_set_viewport(cmd, &viewport);

  hina_scissor scissor = {};
  scissor.width = width;
  scissor.height = height;
  hina_cmd_set_scissor(cmd, &scissor);

  // Bind pipeline
  hina_cmd_bind_pipeline(cmd, m_compositePipeline.get());

  // Bind both bind groups: set 0 = G-buffer textures, set 1 = light UBO
  hina_cmd_bind_group(cmd, 0, m_compositeBindGroup);
  hina_cmd_bind_group(cmd, 1, m_lightBindGroup);

  // Bind fullscreen quad vertex buffer
  hina_vertex_input vertInput = {};
  vertInput.vertex_buffers[0] = m_fullscreenQuadVB;
  vertInput.vertex_offsets[0] = 0;
  hina_cmd_apply_vertex_input(cmd, &vertInput);

  // Draw fullscreen quad (6 vertices)
  hina_cmd_draw(cmd, 6, 1, 0, 0);
}

