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
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cassert>

// =============================================================================
// Shader Loading Helper with VFS Include Support
// =============================================================================

static bool LoadShaderSource(const char* vfsPath, std::string& outSource)
{
  if (!VFS::ReadFile(vfsPath, outSource)) {
    LOG_ERROR("[SceneRenderFeature] Failed to load shader: {}", vfsPath);
    return false;
  }
  return true;
}

// VFS-based include loader callback for HSL compiler
// Returns heap-allocated string that HinaVK will free
static char* VFSIncludeLoader(const char* includePath, const char* includingFile, void* /*userData*/)
{
  // Resolve include path relative to the including file's directory
  std::string baseDir = VFS::GetParentPath(includingFile);
  std::string fullPath = VFS::JoinPath(baseDir, includePath);

  std::string content;
  if (!VFS::ReadFile(fullPath, content)) {
    LOG_ERROR("[SceneRenderFeature] Failed to load include: {} (from {})", fullPath, includingFile);
    return nullptr;
  }

  // Return heap-allocated copy (HinaVK will free it)
  char* result = static_cast<char*>(malloc(content.size() + 1));
  if (result) {
    memcpy(result, content.c_str(), content.size() + 1);
  }
  return result;
}

// Compile HSL shader with VFS include support
static hina_hsl_module* CompileHSLWithIncludes(const char* vfsPath, const char* moduleName, char** outError)
{
  std::string source;
  if (!LoadShaderSource(vfsPath, source)) {
    return nullptr;
  }

  hslc_hsl_module_desc desc = {};
  desc.source = source.c_str();
  desc.source_name = vfsPath;
  desc.load_include_fn = VFSIncludeLoader;
  desc.user_data = nullptr;

  return hslc_compile_hsl_module_ex(&desc, outError);
}

// Shader file paths (relative to VFS mount point, which maps "" -> "./Assets")
static const char* kGBufferShaderPath = "shaders/gbuffer.hina_sl";
static const char* kGBufferSkinnedShaderPath = "shaders/gbuffer_skinned.hina_sl";
static const char* kGBufferMorphedShaderPath = "shaders/gbuffer_skinned_morphed.hina_sl";
static const char* kCompositeTileShaderPath = "shaders/composite_tile.hina_sl";
static const char* kWBOITAccumShaderPath = "shaders/wboit_accum.hina_sl";
static const char* kWBOITAccumSkinnedShaderPath = "shaders/wboit_accum_skinned.hina_sl";
static const char* kWBOITResolveShaderPath = "shaders/wboit_resolve.hina_sl";
static const char* kWBOITPickShaderPath = "shaders/wboit_pick.hina_sl";
static const char* kWBOITPickSkinnedShaderPath = "shaders/wboit_pick_skinned.hina_sl";

// =============================================================================
// Tile Pass Layout for Deferred Rendering
// =============================================================================
// Describes the subpass structure for G-buffer + composite as a tile pass:
//   Subpass 0: G-Buffer (4 color outputs + depth write)
//   Subpass 1: Composite (1 color output, 3 tile inputs from subpass 0, depth read-only)
// =============================================================================

static hina_tile_pass_layout GetDeferredTilePassLayout()
{
  hina_tile_pass_layout layout = {};
  layout.samples = HINA_SAMPLE_COUNT_1_BIT;

  // Subpass 0: G-Buffer fill (4 color outputs + depth)
  layout.subpasses[0].color_count = 4;
  layout.subpasses[0].color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;    // Albedo
  layout.subpasses[0].color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;     // Normal (octahedral)
  layout.subpasses[0].color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;    // Material data
  layout.subpasses[0].color_formats[3] = HINA_FORMAT_R32_UINT;          // Visibility ID
  layout.subpasses[0].depth_format = HINA_FORMAT_D32_SFLOAT;
  layout.subpasses[0].depth_read_only = false;  // G-buffer writes depth
  layout.subpasses[0].input_count = 0;

  // Subpass 1: Composition (1 color output, 3 tile inputs from subpass 0, depth input)
  layout.subpasses[1].color_count = 1;
  layout.subpasses[1].color_formats[0] = HINA_FORMAT_R16G16B16A16_SFLOAT;  // HDR scene color
  layout.subpasses[1].depth_format = HINA_FORMAT_D32_SFLOAT;
  layout.subpasses[1].depth_read_only = true;  // Composite doesn't write depth
  layout.subpasses[1].depth_input = true;      // Read depth as tile input for world pos reconstruction
  layout.subpasses[1].input_count = 3;  // Albedo, normal, materialData from subpass 0
  // Tile input mappings: source subpass and attachment index
  layout.subpasses[1].tile_inputs[0] = {0, 0};  // Albedo from subpass 0, attachment 0
  layout.subpasses[1].tile_inputs[1] = {0, 1};  // Normal from subpass 0, attachment 1
  layout.subpasses[1].tile_inputs[2] = {0, 2};  // MaterialData from subpass 0, attachment 2
  // Note: VisibilityID (attachment 3) is NOT a tile input - only used for picking
  // Note: Depth is at tile_input index 3 (after color inputs) via depth_input flag

  return layout;
}


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
  m_gbufferMorphedPipeline.reset();
  m_sceneLayout.reset();
  m_pipelineCreated = false;
  m_skinnedPipelineCreated = false;
  m_morphedPipelineCreated = false;

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

  // Skybox resources
  if (hina_bind_group_is_valid(m_skyboxBindGroup)) {
    hina_destroy_bind_group(m_skyboxBindGroup);
    m_skyboxBindGroup = {};
  }
  m_skyboxLayout.reset();
  m_skyboxSampler.reset();
  m_lastSkyboxTexture = {};
  m_skyboxBindGroupCreated = false;
  // Destroy fallback skybox cubemap
  if (hina_texture_is_valid(m_fallbackSkyboxTexture)) {
    hina_destroy_texture(m_fallbackSkyboxTexture);
    m_fallbackSkyboxTexture = {};
    m_fallbackSkyboxView = {};
  }

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
  m_wboitAccumSkinnedPipeline.reset();
  m_wboitResolvePipeline.reset();
  m_wboitFrameLayout.reset();
  m_wboitResolveLayout.reset();
  m_wboitSampler.reset();
  m_wboitPipelinesCreated = false;
  m_lastWboitAccum = {};

  // Picking resources
  if (hina_buffer_is_valid(m_pickStagingBuffer)) {
    hina_destroy_buffer(m_pickStagingBuffer);
    m_pickStagingBuffer = {};
  }
  m_pickStagingMapped = nullptr;
  m_pickReadbackPending = false;

  m_drawList.clear();
  m_transparentDrawList.clear();
  m_lightList.clear();
  m_entityHandles.clear();
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

  // Compile shader with VFS include support
  char* error = nullptr;
  hina_hsl_module* module = CompileHSLWithIncludes(kGBufferShaderPath, "scene_gbuffer", &error);
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

  // Get tile pass layout for subpass 0 (G-buffer)
  hina_tile_pass_layout tileLayout = GetDeferredTilePassLayout();

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_BACK;  // Back-face culling enabled
  pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;  // CCW = front (Vulkan/glTF standard)
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = true;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  // Color formats - first UNDEFINED terminates the list
  pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;
  pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;
  pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;
  pip_desc.color_formats[3] = HINA_FORMAT_R32_UINT;  // Visibility buffer for object picking
  // Set 0: Frame UBO, Set 1: Material (constants UBO + textures), Set 2: Dynamic transform UBO
  // Bind group layouts - first invalid handle terminates the list
  pip_desc.bind_group_layouts[0] = m_sceneLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  // Tile pass layout for deferred rendering (subpass 0)
  pip_desc.tile_layout = &tileLayout;
  pip_desc.subpass_index = 0;

  m_gbufferPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_gbufferPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Pipeline creation failed");
    return false;
  }

  // Create persistent frame UBO (viewProj matrix = 64 bytes)
  hina_buffer_desc ubo_desc = {};
  ubo_desc.size = sizeof(glm::mat4);
  ubo_desc.memory = HINA_BUFFER_CPU;
  ubo_desc.usage = HINA_BUFFER_UNIFORM;
  m_frameUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_frameUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create frame UBO");
    return false;
  }

  // Persistently map the UBO for fast per-frame updates
  m_frameUBOMapped = hina_mapped_buffer_ptr(m_frameUBO);
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

  // Compile shader with VFS include support
  char* error = nullptr;
  hina_hsl_module* module = CompileHSLWithIncludes(kGBufferSkinnedShaderPath, "scene_gbuffer_skinned", &error);
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

  // Get tile pass layout for subpass 0 (G-buffer)
  hina_tile_pass_layout tileLayout = GetDeferredTilePassLayout();

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_BACK;  // Back-face culling enabled
  pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;  // CCW = front (Vulkan/glTF standard)
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = true;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  // Color formats - first UNDEFINED terminates the list
  pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;
  pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;
  pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;
  pip_desc.color_formats[3] = HINA_FORMAT_R32_UINT;  // Visibility buffer for object picking
  // Bind group layouts - first invalid handle terminates the list
  pip_desc.bind_group_layouts[0] = m_sceneLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  // Tile pass layout for deferred rendering (subpass 0)
  pip_desc.tile_layout = &tileLayout;
  pip_desc.subpass_index = 0;

  m_gbufferSkinnedPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_gbufferSkinnedPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Skinned pipeline creation failed");
    return false;
  }

  m_skinnedPipelineCreated = true;
  return true;
}

bool SceneRenderFeature::EnsureMorphedPipelineCreated(GfxRenderer* gfxRenderer)
{
  if (m_morphedPipelineCreated) {
    return true;
  }

  // Skinned pipeline must be created first (morph extends skinned)
  if (!m_skinnedPipelineCreated) {
    if (!EnsureSkinnedPipelineCreated(gfxRenderer)) {
      return false;
    }
  }

  // Compile shader with VFS include support
  char* error = nullptr;
  hina_hsl_module* module = CompileHSLWithIncludes(kGBufferMorphedShaderPath, "scene_gbuffer_morphed", &error);
  if (!module) {
    LOG_WARNING("[SceneRenderFeature] Morphed shader compilation failed: {}, falling back to skinned",
             error ? error : "unknown");
    if (error) hslc_free_log(error);
    m_morphedPipelineCreated = true;  // Mark as "created" to avoid repeated attempts
    return true;
  }

  // Material layout from GfxRenderer (Set 1: constants UBO + textures)
  gfx::BindGroupLayout materialLayout = gfxRenderer->getMaterialLayout();
  // Dynamic layout from GfxRenderer (Set 2: transform UBO + bone matrices + morph weights)
  gfx::BindGroupLayout dynamicLayout = gfxRenderer->getDynamicLayout();

  // Vertex layout for split streams (4 buffers for morphed meshes)
  // Note: Buffer 3 is for pre-blended morph deltas (CPU-computed)
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 4;
  // Buffer 0: Position stream (12 bytes)
  vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition);
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 1: Attribute stream (12 bytes)
  vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes);
  vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 2: Skinning stream (8 bytes)
  vertex_layout.buffer_strides[2] = sizeof(gfx::VertexSkinning);
  vertex_layout.input_rates[2] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 3: Morph delta stream (12 bytes - vec3)
  vertex_layout.buffer_strides[3] = 12;  // sizeof(vec3)
  vertex_layout.input_rates[3] = HINA_VERTEX_INPUT_RATE_VERTEX;

  // 7 attributes total
  vertex_layout.attr_count = 7;
  // Buffer 0 attributes
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };  // position
  // Buffer 1 attributes
  vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };   // normalX
  vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };   // packed
  vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };   // uv
  // Buffer 2 attributes (skinning)
  vertex_layout.attrs[4] = { HINA_FORMAT_R8G8B8A8_UINT, 0, 4, 2 };    // boneIndices
  vertex_layout.attrs[5] = { HINA_FORMAT_R8G8B8A8_UNORM, 4, 5, 2 };   // boneWeights
  // Buffer 3 attributes (morph delta)
  vertex_layout.attrs[6] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 6, 3 }; // morphDelta

  // Get tile pass layout for subpass 0 (G-buffer)
  hina_tile_pass_layout tileLayout = GetDeferredTilePassLayout();

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_BACK;  // Back-face culling enabled
  pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;  // CCW = front (Vulkan/glTF standard)
  pip_desc.depth.depth_test = true;
  pip_desc.depth.depth_write = true;
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  // Color formats - first UNDEFINED terminates the list
  pip_desc.color_formats[0] = HINA_FORMAT_R8G8B8A8_SRGB;
  pip_desc.color_formats[1] = HINA_FORMAT_R16G16_SFLOAT;
  pip_desc.color_formats[2] = HINA_FORMAT_R8G8B8A8_UNORM;
  pip_desc.color_formats[3] = HINA_FORMAT_R32_UINT;  // Visibility buffer for object picking
  // Bind group layouts - first invalid handle terminates the list
  pip_desc.bind_group_layouts[0] = m_sceneLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  // Tile pass layout for deferred rendering (subpass 0)
  pip_desc.tile_layout = &tileLayout;
  pip_desc.subpass_index = 0;

  m_gbufferMorphedPipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_gbufferMorphedPipeline.get())) {
    LOG_WARNING("[SceneRenderFeature] Morphed pipeline creation failed, falling back to skinned");
    m_morphedPipelineCreated = true;  // Mark as "created" to avoid repeated attempts
    return true;
  }

  LOG_INFO("[SceneRenderFeature] Morphed pipeline created successfully");
  m_morphedPipelineCreated = true;
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
  feature->m_entityHandles.clear();

  // Reset overflow tracking for the new frame
  feature->m_transformOverflowWarned = false;
  feature->m_boneOverflowWarned = false;
  feature->m_droppedDrawsThisFrame = 0;

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

    // Two-level frustum culling: test model-level AABB first when multiple submeshes
    bool skipPerSubmeshCull = false;
    if (mesh->handles.size() > 1) {
      BoundingBox modelBounds(vec3(FLT_MAX), vec3(-FLT_MAX));
      bool hasAnyValid = false;
      for (size_t i = 0; i < mesh->handles.size(); ++i) {
        const MeshHandle& mh = mesh->handles[i];
        if (!mh.isValid()) continue;
        const MeshGPUData* md = resourceMngr.getMesh(mh);
        if (!md || !md->hasGfxMesh) continue;
        const gfx::GpuMesh* gm = renderer.getMeshStorage().get(
            gfx::MeshHandle{static_cast<uint16_t>(md->gfxMeshIndex),
                            static_cast<uint16_t>(md->gfxMeshGeneration)});
        if (!gm || !gm->valid) continue;
        BoundingBox submeshBounds(gm->bounds.aabbMin, gm->bounds.aabbMax);
        submeshBounds.transform(entityWorldMatrix * mesh->transforms[i]);
        modelBounds.combinePoint(submeshBounds.min_);
        modelBounds.combinePoint(submeshBounds.max_);
        hasAnyValid = true;
      }
      if (hasAnyValid) {
        if (hasValidSkinning) {
          vec3 center = modelBounds.getCenter();
          vec3 halfSize = modelBounds.getSize() * 0.5f * 1.5f;
          modelBounds.min_ = center - halfSize;
          modelBounds.max_ = center + halfSize;
        }
        vec3 bMin = modelBounds.min_, bMax = modelBounds.max_;
        auto result = renderer.classifyInFrustum(bMin, bMax);
        if (result == CPUCuller::FrustumResult::Outside) continue;
        if (result == CPUCuller::FrustumResult::FullyInside) skipPerSubmeshCull = true;
      }
    }

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
      if (!skipPerSubmeshCull && gpuMesh && gpuMesh->valid) {
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

        // Set morph data if entity has active morphs
        if (animComp->morphCount > 0 && !animComp->morphWeights.empty()) {
          draw.hasMorphs = true;
          draw.morphCount = static_cast<uint32_t>(animComp->morphWeights.size());
          draw.morphWeights = animComp->morphWeights.data();
        }
      }

      const ResourceMaterial* material = nullptr;
      if (i < materialCount) {
        material = materialList[i].GetResource();
      }
      if (!material) {
        material = AssetManager::GetContainer<ResourceMaterial>().GetResource(
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

      // Assign object ID for picking (index into m_entityHandles)
      draw.objectId = static_cast<uint32_t>(feature->m_entityHandles.size());
      feature->m_entityHandles.push_back(entity);

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
  // =========================================================================
  // Deferred Rendering with Tile Pass
  // G-buffer and composite are combined into a single tile pass with subpasses.
  // This is more efficient on tile-based GPUs (mobile).
  // =========================================================================

  // Combined tile pass: G-buffer (subpass 0) + Composite (subpass 1)
  // This pass writes to both SCENE_DEPTH and SCENE_COLOR
  passBuilder.CreatePass()
    .SetPriority(internal::RenderPassBuilder::PassPriority::Opaque)
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Write)
    .UseResource(RenderResources::SCENE_COLOR, AccessType::Write)
    .AddGenericPass("SceneDeferredTilePass", [this](internal::ExecutionContext& ctx) {
      ExecuteDeferredTilePass(ctx);
    });

  // =========================================================================
  // WBOIT Passes (Order-Independent Transparency)
  // =========================================================================
  // WBOIT Accumulation pass - renders transparent objects with weighted blending
  // Uses SCENE_DEPTH for depth testing (read-only), writes to WBOIT_Accumulation (owned by GfxRenderer)
  // Priority 510: Must run AFTER deferred lighting is complete
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::Read)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(510))
    .ExecuteAfter("SceneDeferredTilePass")
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

  // =========================================================================
  // Object Picking (Transparent Support)
  // =========================================================================
  // When a pick is requested, re-render transparents into VisibilityID with depth writes enabled
  // so the front-most transparent surface wins.
  passBuilder.CreatePass()
    .UseResource(RenderResources::SCENE_DEPTH, AccessType::ReadWrite)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(530))
    .ExecuteAfter("WBOITResolve")
    .AddGenericPass("TransparentPicking", [this](internal::ExecutionContext& ctx) {
      ExecuteTransparentPicking(ctx);
    });

  // Object picking readback pass - must run after TransparentPicking writes VisibilityID.
  passBuilder.CreatePass()
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(540))
    .ExecuteAfter("TransparentPicking")
    .AddGenericPass("ObjectPicking", [this](internal::ExecutionContext& ctx) {
      ProcessPendingPick(ctx);
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

  // Visibility ID for object picking (R32_UINT)
  // Clear to 0 (no object - shader outputs objectId+1, so 0 means background)
  pass.colors[3].image = gbuffer.visibilityIDView;
  pass.colors[3].load_op = HINA_LOAD_OP_CLEAR;
  pass.colors[3].store_op = HINA_STORE_OP_STORE;

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

  // projMatrix already has Vulkan Y-flip applied at source (SetViewCamera in GraphicsAPI.cpp)
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

    // Separate draws into static, skinned, and morphed
    std::vector<const DrawData*> staticDraws;
    std::vector<const DrawData*> skinnedDraws;
    std::vector<const DrawData*> morphedDraws;
    staticDraws.reserve(m_drawList.size());
    skinnedDraws.reserve(32);   // Typical skinned draw count
    morphedDraws.reserve(16);   // Typical morphed draw count

    for (const auto& draw : m_drawList) {
      // Only classify as skinned if mesh also has skinning data
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      bool meshHasSkinning = gpuMesh && gpuMesh->valid && gpuMesh->hasSkinning;
      bool meshHasMorphs = gpuMesh && gpuMesh->valid && gpuMesh->hasMorphs;

      if (draw.isSkinned && draw.skinMatrices && meshHasSkinning) {
        // Morphed meshes go to separate bucket (morphs + skinning)
        if (draw.hasMorphs && draw.morphWeights && meshHasMorphs) {
          morphedDraws.push_back(&draw);
        } else {
          skinnedDraws.push_back(&draw);
        }
      } else {
        staticDraws.push_back(&draw);
      }
    }

    // Sort each bucket by material to minimize bind group switches
    auto sortByMaterial = [](const DrawData* a, const DrawData* b) {
      // Sort by material handle (index, then generation)
      if (a->gfxMaterial.index != b->gfxMaterial.index)
        return a->gfxMaterial.index < b->gfxMaterial.index;
      return a->gfxMaterial.generation < b->gfxMaterial.generation;
    };
    std::sort(staticDraws.begin(), staticDraws.end(), sortByMaterial);
    std::sort(skinnedDraws.begin(), skinnedDraws.end(), sortByMaterial);
    std::sort(morphedDraws.begin(), morphedDraws.end(), sortByMaterial);

    // =========================================================================
    // Pass 1: Render static meshes with m_gbufferPipeline
    // =========================================================================
    gfx::BindGroup lastMaterialBG = {};  // Track last bound material to skip redundant binds

    for (const DrawData* drawPtr : staticDraws) {
      const DrawData& draw = *drawPtr;
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      if (!gpuMesh || !gpuMesh->valid) continue;

      // Check transform ring buffer overflow
      if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
        if (!m_transformOverflowWarned) {
        //  LOG_WARN("[SceneRenderFeature] Transform ring buffer overflow! Some objects will not be rendered. "
       //            "Consider reducing draw count or increasing MAX_TRANSFORMS (current: {})", MAX_TRANSFORMS);
          m_transformOverflowWarned = true;
        }
        ++m_droppedDrawsThisFrame;
        continue;  // Skip this draw but continue checking others (for counting)
      }

      // Write transform, objectId, and flags to ring buffer at current offset
      uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
      glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
      *transformDst = draw.transform;
      // Write objectId at offset 64 (after mat4)
      uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
      *objectIdDst = draw.objectId;
      // Write flags at offset 68 - bit 0 = negative determinant (for mirrored transforms)
      uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
      float det = glm::determinant(draw.transform);
      *flagsDst = (det < 0.0f) ? 1u : 0u;

      // Bind split vertex streams with offsets (2 buffers for static)
      hina_vertex_input meshInput = {};
      meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
      meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
      meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
      meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
      meshInput.index_buffer = gpuMesh->indexBuffer;
      meshInput.index_type = HINA_INDEX_UINT32;

      hina_cmd_apply_vertex_input(cmd, &meshInput);

      // Bind material at Set 1 (skip if same as last)
      gfx::BindGroup materialBG;
      if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
        materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
      } else {
        materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
      }
      if (materialBG.id != lastMaterialBG.id) {
        hina_cmd_bind_group(cmd, 1, materialBG);
        lastMaterialBG = materialBG;
      }

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
        // Reset material tracking for new pipeline
        lastMaterialBG = {};
        // Switch to skinned pipeline
        hina_cmd_bind_pipeline(cmd, m_gbufferSkinnedPipeline.get());
        hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

        for (const DrawData* drawPtr : skinnedDraws) {
          const DrawData& draw = *drawPtr;
          const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
          if (!gpuMesh || !gpuMesh->valid) continue;

          // Check buffer overflow
          if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
            if (!m_transformOverflowWarned) {
              //LOG_WARN("[SceneRenderFeature] Transform ring buffer overflow (skinned)! "
              //         "Some objects will not be rendered.");
              m_transformOverflowWarned = true;
            }
            ++m_droppedDrawsThisFrame;
            continue;
          }
          if (frame.boneMatrixRingOffset >= BONE_MATRIX_UBO_SIZE) {
            if (!m_boneOverflowWarned) {
              //LOG_WARN("[SceneRenderFeature] Bone matrix ring buffer overflow! "
              //         "Some skinned objects will not be rendered.");
              m_boneOverflowWarned = true;
            }
            ++m_droppedDrawsThisFrame;
            continue;
          }

          // Write transform, objectId, and flags to ring buffer
          uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
          glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
          *transformDst = draw.transform;
          // Write objectId at offset 64 (after mat4)
          uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
          *objectIdDst = draw.objectId;
          // Write flags at offset 68 - bit 0 = negative determinant
          uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
          float det = glm::determinant(draw.transform);
          *flagsDst = (det < 0.0f) ? 1u : 0u;

          // Write bone matrices to ring buffer (up to MAX_BONES_PER_MESH)
          // Initialize all slots to identity first, then copy actual bones
          uint32_t numBones = std::min(draw.jointCount, gfx::MAX_BONES_PER_MESH);
          if (draw.jointCount > gfx::MAX_BONES_PER_MESH) {
            static bool warnedOnce = false;
            if (!warnedOnce) {
              //LOG_WARNING("[SceneRenderFeature] Mesh has {} bones but MAX_BONES_PER_MESH is {}. "
              //            "Bones beyond limit will use identity matrices. Consider bone remapping.",
                //          draw.jointCount, gfx::MAX_BONES_PER_MESH);
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

          // Bind material at Set 1 (skip if same as last)
          gfx::BindGroup materialBG;
          if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
            materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
          } else {
            materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
          }
          if (materialBG.id != lastMaterialBG.id) {
            hina_cmd_bind_group(cmd, 1, materialBG);
            lastMaterialBG = materialBG;
          }

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

    // =========================================================================
    // Pass 3: Render morphed meshes (skinned + morph targets)
    // NOTE: Currently uses skinned pipeline as fallback since morphed pipeline
    //       requires CPU pre-blended morph deltas (per-frame vertex buffer).
    //       Future work: Add morph weight UBO pool + morph delta ring buffer.
    // =========================================================================
    if (!morphedDraws.empty()) {
      // Ensure skinned pipeline is available (morph pipeline falls back to skinned)
      if (!EnsureSkinnedPipelineCreated(gfxRenderer) || !frame.boneMatrixRingMapped) {
        // Skip morphed draws if pipeline or bone buffer not available
      } else {
        // Reset material tracking for pipeline (may have been set by skinned pass)
        lastMaterialBG = {};
        // Use skinned pipeline for now (morphed pipeline requires additional infrastructure)
        // TODO: When morphed pipeline is ready, use m_gbufferMorphedPipeline and bind
        //       morph weight UBO + pre-blended delta vertex buffer
        hina_cmd_bind_pipeline(cmd, m_gbufferSkinnedPipeline.get());
        hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

        for (const DrawData* drawPtr : morphedDraws) {
          const DrawData& draw = *drawPtr;
          const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
          if (!gpuMesh || !gpuMesh->valid) continue;

          // Check buffer overflow
          if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
            if (!m_transformOverflowWarned) {
              //LOG_WARN("[SceneRenderFeature] Transform ring buffer overflow (morphed)! "
              //         "Some objects will not be rendered.");
              m_transformOverflowWarned = true;
            }
            ++m_droppedDrawsThisFrame;
            continue;
          }
          if (frame.boneMatrixRingOffset >= BONE_MATRIX_UBO_SIZE) {
            if (!m_boneOverflowWarned) {
              //LOG_WARN("[SceneRenderFeature] Bone matrix ring buffer overflow (morphed)! "
              //         "Some objects will not be rendered.");
              m_boneOverflowWarned = true;
            }
            ++m_droppedDrawsThisFrame;
            continue;
          }

          // Write transform, objectId, and flags to ring buffer
          uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
          glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
          *transformDst = draw.transform;
          uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
          *objectIdDst = draw.objectId;
          // Write flags at offset 68 - bit 0 = negative determinant
          uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
          float det = glm::determinant(draw.transform);
          *flagsDst = (det < 0.0f) ? 1u : 0u;

          // Write bone matrices to ring buffer
          uint32_t numBones = std::min(draw.jointCount, gfx::MAX_BONES_PER_MESH);
          glm::mat4* bonesDst = reinterpret_cast<glm::mat4*>(
              static_cast<uint8_t*>(frame.boneMatrixRingMapped) + frame.boneMatrixRingOffset);

          static const glm::mat4 identity(1.0f);
          for (uint32_t i = 0; i < gfx::MAX_BONES_PER_MESH; ++i) {
            bonesDst[i] = identity;
          }
          if (draw.skinMatrices) {
            std::memcpy(bonesDst, draw.skinMatrices, numBones * sizeof(glm::mat4));
          }

          // TODO: When morphed pipeline is ready:
          // 1. Compute pre-blended morph deltas on CPU using draw.morphWeights
          // 2. Upload to per-frame morph delta ring buffer
          // 3. Bind buffer 3 with morph delta vertex buffer
          // For now, morph weights are ignored (mesh renders without morphs)

          // Bind split vertex streams (3 buffers - same as skinned, no morph delta)
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
          if (materialBG.id != lastMaterialBG.id) {
            hina_cmd_bind_group(cmd, 1, materialBG);
            lastMaterialBG = materialBG;
          }

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

ecs::EntityHandle SceneRenderFeature::GetEntityAtDrawIndex(uint32_t drawIndex) const
{
  if (drawIndex < m_entityHandles.size()) {
    return m_entityHandles[drawIndex];
  }
  return nullptr;
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

  // Sort by isSkinned (pipeline), then by material to minimize state switches
  std::sort(m_transparentDrawList.begin(), m_transparentDrawList.end(),
            [](const DrawData& a, const DrawData& b) {
              if (a.isSkinned != b.isSkinned) return a.isSkinned < b.isSkinned;
              if (a.gfxMaterial.index != b.gfxMaterial.index)
                return a.gfxMaterial.index < b.gfxMaterial.index;
              return a.gfxMaterial.generation < b.gfxMaterial.generation;
            });

  const FrameData& frameData = ctx.GetFrameData();
  gfx::Cmd* cmd = ctx.GetCmd();
  const auto& meshStorage = gfxRenderer->getMeshStorage();
  const auto& materialSystem = gfxRenderer->getMaterialSystem();
  const auto& wboit = gfxRenderer->getWBOIT();
  auto& frame = gfxRenderer->getCurrentFrameResources();

  // Get scene depth for depth testing
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Begin WBOIT accumulation pass (single color target)
  hina_pass_action pass = {};
  // Target 0: Accumulation (RGBA16F) - cleared to 0
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

  // Track state to minimize switches
  // Note: frame and light bind groups are bound after pipeline is bound (in the loop)
  bool lastWasSkinned = false;
  bool pipelineBound = false;
  gfx::BindGroup lastMaterialBG = {};

  // Clear and prepare to store offsets for TransparentPicking to reuse
  m_wboitDrawOffsets.clear();
  m_wboitDrawOffsets.reserve(m_transparentDrawList.size());

  // Render each transparent draw
  for (const DrawData& draw : m_transparentDrawList) {
    const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
    if (!gpuMesh || !gpuMesh->valid) continue;

    // Check transform ring buffer overflow
    // Note: Must break (not continue) to keep m_wboitDrawOffsets in sync for TransparentPicking
    if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
      if (!m_transformOverflowWarned) {
        //LOG_WARN("[SceneRenderFeature] Transform ring buffer overflow (WBOIT)! "
        //         "Some transparent objects will not be rendered or pickable. "
        //         "Drew {}/{} transparent objects.", m_wboitDrawOffsets.size(), m_transparentDrawList.size());
        m_transformOverflowWarned = true;
      }
      break;
    }

    // Bind appropriate pipeline based on skinning
    if (!pipelineBound || draw.isSkinned != lastWasSkinned) {
      if (draw.isSkinned) {
        hina_cmd_bind_pipeline(cmd, m_wboitAccumSkinnedPipeline.get());
      } else {
        hina_cmd_bind_pipeline(cmd, m_wboitAccumPipeline.get());
      }
      lastWasSkinned = draw.isSkinned;
      pipelineBound = true;

      // Re-bind bind groups after pipeline switch (Vulkan requirement)
      hina_cmd_bind_group(cmd, 0, m_wboitFrameBindGroup);
      if (hina_bind_group_is_valid(m_lightBindGroup)) {
        hina_cmd_bind_group(cmd, 3, m_lightBindGroup);
      }
      // Reset material tracking on pipeline switch
      lastMaterialBG = {};
    }

    // Write transform, objectId, and flags to ring buffer
    uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
    glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
    *transformDst = draw.transform;
    // Write objectId at offset 64 (after mat4) - for consistency with G-buffer shader UBO layout
    uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
    *objectIdDst = draw.objectId;
    // Write flags at offset 68 - bit 0 = negative determinant
    uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
    float det = glm::determinant(draw.transform);
    *flagsDst = (det < 0.0f) ? 1u : 0u;

    // Handle bone matrices for skinned meshes
    uint32_t boneOffset = 0;
    if (draw.isSkinned && draw.skinMatrices && draw.jointCount > 0) {
      // Check bone ring buffer overflow
      size_t boneDataSize = draw.jointCount * sizeof(glm::mat4);
      size_t alignedBoneSize = (boneDataSize + 255) & ~255;  // 256-byte alignment

      if (frame.boneMatrixRingOffset + alignedBoneSize <= BONE_MATRIX_UBO_SIZE) {
        boneOffset = frame.boneMatrixRingOffset;

        // Upload bone matrices to ring buffer
        glm::mat4* boneDst = reinterpret_cast<glm::mat4*>(
            static_cast<uint8_t*>(frame.boneMatrixRingMapped) + frame.boneMatrixRingOffset);
        std::memcpy(boneDst, draw.skinMatrices, boneDataSize);

        frame.boneMatrixRingOffset += static_cast<uint32_t>(alignedBoneSize);
      }
    }

    // Bind vertex streams
    hina_vertex_input meshInput = {};
    meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
    meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
    meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
    meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
    meshInput.index_buffer = gpuMesh->indexBuffer;
    meshInput.index_type = HINA_INDEX_UINT32;

    // For skinned meshes, also bind the skinning buffer
    if (draw.isSkinned && gpuMesh->hasSkinning) {
      meshInput.vertex_buffers[2] = gpuMesh->skinningBuffer;
      meshInput.vertex_offsets[2] = gpuMesh->skinningOffset;
    }

    hina_cmd_apply_vertex_input(cmd, &meshInput);

    // Bind material at Set 1 (skip if same as last)
    gfx::BindGroup materialBG;
    if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
      materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
    } else {
      materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
    }
    if (materialBG.id != lastMaterialBG.id) {
      hina_cmd_bind_group(cmd, 1, materialBG);
      lastMaterialBG = materialBG;
    }

    // Bind Set 2 with dynamic offsets (transform + bones)
    uint32_t dynamicOffsets[2] = { frame.transformRingOffset, boneOffset };
    hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);

    // Store offsets for TransparentPicking to reuse
    m_wboitDrawOffsets.push_back({ frame.transformRingOffset, boneOffset });

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

  // Ensure resolve bind group is created/updated (only needs accumulation texture)
  bool needsRecreate = !hina_bind_group_is_valid(m_wboitResolveBindGroup)
                    || wboit.accumulation.id != m_lastWboitAccum.id;

  if (needsRecreate) {
    if (hina_bind_group_is_valid(m_wboitResolveBindGroup)) {
      hina_destroy_bind_group(m_wboitResolveBindGroup);
      m_wboitResolveBindGroup = {};
    }

    hina_bind_group_entry entries[1] = {};
    // Binding 0: Accumulation texture
    entries[0].binding = 0;
    entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    entries[0].combined.view = wboit.accumulationView;
    entries[0].combined.sampler = m_wboitSampler.get();

    hina_bind_group_desc bg_desc = {};
    bg_desc.layout = m_wboitResolveLayout.get();
    bg_desc.entries = entries;
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

void SceneRenderFeature::ExecuteTransparentPicking(internal::ExecutionContext& ctx)
{
  if (!m_enableObjectPicking) {
    return;
  }

  // Only run this pass when a pick is actually requested (avoids extra work and depth writes).
  {
    std::lock_guard<std::mutex> lock(m_pickResultMutex);
    if (!m_pickRequest.pending) {
      return;
    }
  }

  // Skip if no transparent draws
  if (m_transparentDrawList.empty()) {
    LOG_DEBUG("[SceneRenderFeature] TransparentPicking: no transparent objects to pick");
    return;
  }

  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  auto& frame = gfxRenderer->getCurrentFrameResources();
  LOG_DEBUG("[SceneRenderFeature] TransparentPicking: {} transparent objects, ringOffset={}/{}",
            m_transparentDrawList.size(), frame.transformRingOffset, TRANSFORM_UBO_SIZE);

  // Ensure WBOIT (and picking) pipelines are created
  EnsureWBOITPipelines(ctx);
  if (!m_wboitPipelinesCreated) {
    return;
  }
  if (!hina_pipeline_is_valid(m_wboitPickPipeline.get()) || !hina_pipeline_is_valid(m_wboitPickSkinnedPipeline.get())) {
    return;
  }

  // Verify we have stored offsets from WBOIT accumulation
  // (m_wboitDrawOffsets is filled during ExecuteWBOIT, same order as m_transparentDrawList)
  if (m_wboitDrawOffsets.empty()) {
    LOG_DEBUG("[SceneRenderFeature] TransparentPicking: no stored offsets from accumulation");
    return;
  }

  // NOTE: Don't re-sort! Use same order as WBOIT accumulation so we can reuse stored offsets.

  const FrameData& frameData = ctx.GetFrameData();
  gfx::Cmd* cmd = ctx.GetCmd();
  const auto& meshStorage = gfxRenderer->getMeshStorage();
  const auto& materialSystem = gfxRenderer->getMaterialSystem();
  const auto& gbuffer = gfxRenderer->getGBuffer();

  if (!hina_texture_view_is_valid(gbuffer.visibilityIDView)) {
    return;
  }

  // Get scene depth for depth testing (opaque depth already written).
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Begin transparent picking pass:
  // - Load VisibilityID so opaque IDs remain where no transparent fragments are in front.
  // - Load depth so only transparents in front of opaque pass.
  // - Depth write enabled in pipeline so the nearest transparent fragment wins.
  hina_pass_action pass = {};
  pass.colors[0].image = gbuffer.visibilityIDView;
  pass.colors[0].load_op = HINA_LOAD_OP_LOAD;
  pass.colors[0].store_op = HINA_STORE_OP_STORE;

  pass.depth.image = sceneDepthView;
  pass.depth.load_op = HINA_LOAD_OP_LOAD;
  pass.depth.store_op = HINA_STORE_OP_STORE;

  hina_cmd_begin_pass(cmd, &pass);

  // Update WBOIT frame UBO (viewProj + cameraPos). CameraPos is unused by pick shaders but kept for layout.
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

  bool lastWasSkinned = false;
  bool pipelineBound = false;
  gfx::BindGroup lastMaterialBG = {};

  // Iterate using index to match m_wboitDrawOffsets (which was filled in same order)
  size_t offsetIndex = 0;
  for (const DrawData& draw : m_transparentDrawList) {
    const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
    if (!gpuMesh || !gpuMesh->valid) continue;

    // Safety check - must have a stored offset for this draw
    if (offsetIndex >= m_wboitDrawOffsets.size()) {
      LOG_DEBUG("[SceneRenderFeature] TransparentPicking: ran out of stored offsets at draw {}", offsetIndex);
      break;
    }

    if (!pipelineBound || draw.isSkinned != lastWasSkinned) {
      if (draw.isSkinned) {
        hina_cmd_bind_pipeline(cmd, m_wboitPickSkinnedPipeline.get());
      } else {
        hina_cmd_bind_pipeline(cmd, m_wboitPickPipeline.get());
      }
      lastWasSkinned = draw.isSkinned;
      pipelineBound = true;

      // Bind frame bind group (Set 0) after pipeline switch.
      hina_cmd_bind_group(cmd, 0, m_wboitFrameBindGroup);
      lastMaterialBG = {};
    }

    // Bind vertex streams
    hina_vertex_input meshInput = {};
    meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
    meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
    meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
    meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
    meshInput.index_buffer = gpuMesh->indexBuffer;
    meshInput.index_type = HINA_INDEX_UINT32;

    if (draw.isSkinned && gpuMesh->hasSkinning) {
      meshInput.vertex_buffers[2] = gpuMesh->skinningBuffer;
      meshInput.vertex_offsets[2] = gpuMesh->skinningOffset;
    }

    hina_cmd_apply_vertex_input(cmd, &meshInput);

    // Bind material at Set 1 (skip if same as last)
    gfx::BindGroup materialBG;
    if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
      materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
    } else {
      materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
    }
    if (materialBG.id != lastMaterialBG.id) {
      hina_cmd_bind_group(cmd, 1, materialBG);
      lastMaterialBG = materialBG;
    }

    // Reuse offsets from WBOIT accumulation (already written to ring buffer)
    const auto& storedOffsets = m_wboitDrawOffsets[offsetIndex];
    uint32_t dynamicOffsets[2] = { storedOffsets.transformOffset, storedOffsets.boneOffset };
    hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);

    ++offsetIndex;

    hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
  }

  LOG_DEBUG("[SceneRenderFeature] TransparentPicking: drew {} transparent objects", offsetIndex);
  hina_cmd_end_pass(cmd);
}

void SceneRenderFeature::ProcessPendingPick(internal::ExecutionContext& ctx)
{
  // Check if there's a pending pick request
  PickRequest request;
  {
    std::lock_guard<std::mutex> lock(m_pickResultMutex);
    if (!m_pickRequest.pending) {
      return;
    }
    request = m_pickRequest;
    m_pickRequest.pending = false;
  }

  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) {
    return;
  }

  const auto& gbuffer = gfxRenderer->getGBuffer();
  if (!hina_texture_is_valid(gbuffer.visibilityID)) {
    return;
  }

  // Get visibility texture dimensions
  uint32_t width = RenderResources::INTERNAL_WIDTH;
  uint32_t height = RenderResources::INTERNAL_HEIGHT;

  // Validate pick coordinates (screen space -> internal resolution)
  // Note: The caller should transform screen coordinates to internal resolution
  // For now, assume they're already in internal resolution space
  int pickX = request.screenX;
  int pickY = request.screenY;

  if (pickX < 0 || pickX >= static_cast<int>(width) ||
      pickY < 0 || pickY >= static_cast<int>(height)) {
    // Out of bounds - no pick
    std::lock_guard<std::mutex> lock(m_pickResultMutex);
    m_lastPickResult.valid = false;
    return;
  }

  // Download the visibility texture to CPU memory
  // This is a synchronous operation and downloads the entire texture
  // TODO: Optimize by using a compute shader to read a single pixel
  size_t texSize = width * height * sizeof(uint32_t);
  std::vector<uint32_t> visibilityData(width * height);

  // hina_download_texture asserts on failure (API changed from returning bool)
  hina_download_texture(
      gbuffer.visibilityID,
      0,  // mip level 0
      0,  // array layer 0
      visibilityData.data(),
      texSize);

  // Read the pixel at the pick location
  // Note: Y coordinate might need to be flipped depending on coordinate system
  uint32_t pixelIndex = pickY * width + pickX;
  uint32_t visibilityValue = visibilityData[pixelIndex];

  LOG_DEBUG("[SceneRenderFeature] Pick at ({}, {}): visibilityValue={}, transparentCount={}",
            pickX, pickY, visibilityValue, m_transparentDrawList.size());

  // Update pick result
  std::lock_guard<std::mutex> lock(m_pickResultMutex);
  if (visibilityValue == 0) {
    // Background (no object) - shader outputs objectId+1, so 0 means nothing was hit
    m_lastPickResult.valid = false;
    m_lastPickResult.sceneObjectIndex = 0;
  } else {
    // Convert visibility value back to objectId (subtract 1 since shader adds 1)
    m_lastPickResult.valid = true;
    m_lastPickResult.sceneObjectIndex = visibilityValue - 1;
  }
}
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

  // Composite pipeline must be created first (provides m_lightLayout)
  EnsureCompositePipelineCreated(gfxRenderer);
  if (!hina_bind_group_layout_is_valid(m_lightLayout.get())) {
    return;  // Light layout not ready yet
  }

  // =========================================================================
  // Create WBOIT Accumulation Pipeline
  // =========================================================================

  // Compile shader with VFS include support
  char* error = nullptr;
  hina_hsl_module* accumModule = CompileHSLWithIncludes(kWBOITAccumShaderPath, "wboit_accum", &error);
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
  ubo_desc.memory = HINA_BUFFER_CPU;
  ubo_desc.usage = HINA_BUFFER_UNIFORM;
  m_wboitFrameUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_wboitFrameUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create WBOIT frame UBO");
    hslc_hsl_module_free(accumModule);
    return;
  }
  m_wboitFrameUBOMapped = hina_mapped_buffer_ptr(m_wboitFrameUBO);

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
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;  // No culling for transparent objects
  pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;  // CCW = front (Vulkan/glTF standard)
  pip_desc.depth.depth_test = true;   // Test against opaque depth
  pip_desc.depth.depth_write = false; // Don't write depth for transparents
  pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pip_desc.color_formats[0] = HINA_FORMAT_R16G16B16A16_SFLOAT;  // Accumulation (RGBA16F) - single output

  // Additive blending for accumulation - final = src * 1 + dst * 1
  pip_desc.blend[0].enable = true;
  pip_desc.blend[0].src_color = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].dst_color = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].color_op = HINA_BLEND_OP_ADD;
  pip_desc.blend[0].src_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE;
  pip_desc.blend[0].alpha_op = HINA_BLEND_OP_ADD;
  pip_desc.blend[0].color_write_mask = HINA_COLOR_COMPONENT_ALL;

  // Set 0: Frame, Set 1: Material, Set 2: Dynamic, Set 3: Lights
  pip_desc.bind_group_layouts[0] = m_wboitFrameLayout.get();
  pip_desc.bind_group_layouts[1] = materialLayout;
  pip_desc.bind_group_layouts[2] = dynamicLayout;
  pip_desc.bind_group_layouts[3] = m_lightLayout.get();  // Reuse from composite

  m_wboitAccumPipeline.reset(hina_make_pipeline_from_module(accumModule, &pip_desc, nullptr));
  hslc_hsl_module_free(accumModule);

  if (!hina_pipeline_is_valid(m_wboitAccumPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT accumulation pipeline creation failed");
    return;
  }

  // =========================================================================
  // Create WBOIT Skinned Accumulation Pipeline
  // =========================================================================
  hina_hsl_module* accumSkinnedModule = CompileHSLWithIncludes(kWBOITAccumSkinnedShaderPath, "wboit_accum_skinned", &error);
  if (!accumSkinnedModule) {
    LOG_ERROR("[SceneRenderFeature] WBOIT skinned accumulation shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return;
  }

  // Skinned vertex layout (must match gbuffer_skinned: 3 buffers with bone data)
  hina_vertex_layout skinned_vertex_layout = {};
  skinned_vertex_layout.buffer_count = 3;
  // Buffer 0: Position stream (12 bytes)
  skinned_vertex_layout.buffer_strides[0] = sizeof(gfx::VertexPosition);
  skinned_vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 1: Attribute stream (12 bytes)
  skinned_vertex_layout.buffer_strides[1] = sizeof(gfx::VertexAttributes);
  skinned_vertex_layout.input_rates[1] = HINA_VERTEX_INPUT_RATE_VERTEX;
  // Buffer 2: Skinning stream (8 bytes)
  skinned_vertex_layout.buffer_strides[2] = sizeof(gfx::VertexSkinning);
  skinned_vertex_layout.input_rates[2] = HINA_VERTEX_INPUT_RATE_VERTEX;

  // 6 attributes total (same as gbuffer_skinned)
  skinned_vertex_layout.attr_count = 6;
  // Buffer 0 attributes
  skinned_vertex_layout.attrs[0] = { HINA_FORMAT_R32G32B32_SFLOAT, 0, 0, 0 };  // position
  // Buffer 1 attributes
  skinned_vertex_layout.attrs[1] = { HINA_FORMAT_R32_UINT, 0, 1, 1 };   // normalX
  skinned_vertex_layout.attrs[2] = { HINA_FORMAT_R32_UINT, 4, 2, 1 };   // packed
  skinned_vertex_layout.attrs[3] = { HINA_FORMAT_R32_UINT, 8, 3, 1 };   // uv
  // Buffer 2 attributes (skinning)
  skinned_vertex_layout.attrs[4] = { HINA_FORMAT_R8G8B8A8_UINT, 0, 4, 2 };    // boneIndices
  skinned_vertex_layout.attrs[5] = { HINA_FORMAT_R8G8B8A8_UNORM, 4, 5, 2 };   // boneWeights

  hina_hsl_pipeline_desc skinned_pip_desc = hina_hsl_pipeline_desc_default();
  skinned_pip_desc.layout = skinned_vertex_layout;
  skinned_pip_desc.cull_mode = HINA_CULL_MODE_NONE;  // No culling for transparent objects
  skinned_pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;  // CCW = front (Vulkan/glTF standard)
  skinned_pip_desc.depth.depth_test = true;
  skinned_pip_desc.depth.depth_write = false;
  skinned_pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  skinned_pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  skinned_pip_desc.color_formats[0] = HINA_FORMAT_R16G16B16A16_SFLOAT;  // Accumulation (single output)

  // Additive blend for accumulation
  skinned_pip_desc.blend[0].enable = true;
  skinned_pip_desc.blend[0].src_color = HINA_BLEND_FACTOR_ONE;
  skinned_pip_desc.blend[0].dst_color = HINA_BLEND_FACTOR_ONE;
  skinned_pip_desc.blend[0].color_op = HINA_BLEND_OP_ADD;
  skinned_pip_desc.blend[0].src_alpha = HINA_BLEND_FACTOR_ONE;
  skinned_pip_desc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE;
  skinned_pip_desc.blend[0].alpha_op = HINA_BLEND_OP_ADD;
  skinned_pip_desc.blend[0].color_write_mask = HINA_COLOR_COMPONENT_ALL;

  skinned_pip_desc.bind_group_layouts[0] = m_wboitFrameLayout.get();
  skinned_pip_desc.bind_group_layouts[1] = materialLayout;
  skinned_pip_desc.bind_group_layouts[2] = dynamicLayout;
  skinned_pip_desc.bind_group_layouts[3] = m_lightLayout.get();

  m_wboitAccumSkinnedPipeline.reset(hina_make_pipeline_from_module(accumSkinnedModule, &skinned_pip_desc, nullptr));
  hslc_hsl_module_free(accumSkinnedModule);

  if (!hina_pipeline_is_valid(m_wboitAccumSkinnedPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT skinned accumulation pipeline creation failed");
    return;
  }

  // =========================================================================
  // Create WBOIT Resolve Pipeline
  // =========================================================================

  hina_hsl_module* resolveModule = CompileHSLWithIncludes(kWBOITResolveShaderPath, "wboit_resolve", &error);
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

  // Set 0: Accumulation texture only (simplified WBOIT without reveal buffer)
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
  resolve_pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;

  // Compositing blend: final = avgColor * (1 - reveal) + opaque * reveal
  resolve_pip_desc.blend[0].enable = true;
  resolve_pip_desc.blend[0].src_color = HINA_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  resolve_pip_desc.blend[0].dst_color = HINA_BLEND_FACTOR_SRC_ALPHA;
  resolve_pip_desc.blend[0].color_op = HINA_BLEND_OP_ADD;
  resolve_pip_desc.blend[0].src_alpha = HINA_BLEND_FACTOR_ZERO;
  resolve_pip_desc.blend[0].dst_alpha = HINA_BLEND_FACTOR_ONE;
  resolve_pip_desc.blend[0].alpha_op = HINA_BLEND_OP_ADD;
  resolve_pip_desc.blend[0].color_write_mask = HINA_COLOR_COMPONENT_ALL;

  resolve_pip_desc.bind_group_layouts[0] = m_wboitResolveLayout.get();

  m_wboitResolvePipeline.reset(hina_make_pipeline_from_module(resolveModule, &resolve_pip_desc, nullptr));
  hslc_hsl_module_free(resolveModule);

  if (!hina_pipeline_is_valid(m_wboitResolvePipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT resolve pipeline creation failed");
    return;
  }

  // =========================================================================
  // Create WBOIT Transparent Picking Pipelines (VisibilityID)
  // =========================================================================

  hina_hsl_module* pickModule = CompileHSLWithIncludes(kWBOITPickShaderPath, "wboit_pick", &error);
  if (!pickModule) {
    LOG_ERROR("[SceneRenderFeature] WBOIT pick shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return;
  }

  hina_hsl_pipeline_desc pick_pip_desc = hina_hsl_pipeline_desc_default();
  pick_pip_desc.layout = vertex_layout;  // same as gbuffer / wboit_accum (2-buffer split streams)
  pick_pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pick_pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;
  pick_pip_desc.depth.depth_test = true;
  pick_pip_desc.depth.depth_write = true;  // critical: makes front-most transparent win
  pick_pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pick_pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pick_pip_desc.color_formats[0] = HINA_FORMAT_R32_UINT;  // VisibilityID target
  pick_pip_desc.bind_group_layouts[0] = m_wboitFrameLayout.get();
  pick_pip_desc.bind_group_layouts[1] = materialLayout;
  pick_pip_desc.bind_group_layouts[2] = dynamicLayout;

  m_wboitPickPipeline.reset(hina_make_pipeline_from_module(pickModule, &pick_pip_desc, nullptr));
  hslc_hsl_module_free(pickModule);

  if (!hina_pipeline_is_valid(m_wboitPickPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT pick pipeline creation failed");
    return;
  }

  hina_hsl_module* pickSkinnedModule = CompileHSLWithIncludes(kWBOITPickSkinnedShaderPath, "wboit_pick_skinned", &error);
  if (!pickSkinnedModule) {
    LOG_ERROR("[SceneRenderFeature] WBOIT skinned pick shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return;
  }

  hina_hsl_pipeline_desc pick_skinned_pip_desc = hina_hsl_pipeline_desc_default();
  pick_skinned_pip_desc.layout = skinned_vertex_layout;  // 3-buffer split streams (with skinning)
  pick_skinned_pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pick_skinned_pip_desc.front_face = HINA_FRONT_FACE_COUNTER_CLOCKWISE;
  pick_skinned_pip_desc.depth.depth_test = true;
  pick_skinned_pip_desc.depth.depth_write = true;
  pick_skinned_pip_desc.depth.depth_compare = HINA_COMPARE_OP_LESS;
  pick_skinned_pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pick_skinned_pip_desc.color_formats[0] = HINA_FORMAT_R32_UINT;
  pick_skinned_pip_desc.bind_group_layouts[0] = m_wboitFrameLayout.get();
  pick_skinned_pip_desc.bind_group_layouts[1] = materialLayout;
  pick_skinned_pip_desc.bind_group_layouts[2] = dynamicLayout;

  m_wboitPickSkinnedPipeline.reset(hina_make_pipeline_from_module(pickSkinnedModule, &pick_skinned_pip_desc, nullptr));
  hslc_hsl_module_free(pickSkinnedModule);

  if (!hina_pipeline_is_valid(m_wboitPickSkinnedPipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] WBOIT skinned pick pipeline creation failed");
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
    vb_desc.memory = HINA_BUFFER_CPU;
    vb_desc.usage = HINA_BUFFER_VERTEX;
    m_fullscreenQuadVB = hina_make_buffer(&vb_desc);
    if (!hina_buffer_is_valid(m_fullscreenQuadVB)) {
      LOG_ERROR("[SceneRenderFeature] Failed to create fullscreen quad vertex buffer");
      return false;
    }
    void* mapped = hina_mapped_buffer_ptr(m_fullscreenQuadVB);
    std::memcpy(mapped, quadVerts, sizeof(quadVerts));
  }

  // Compile composite shader with VFS include support
  char* error = nullptr;
  hina_hsl_module* module = CompileHSLWithIncludes(kCompositeTileShaderPath, "scene_composite", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Composite shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // =========================================================================
  // Set 0: G-buffer textures (INPUT_ATTACHMENT for tile pass)
  // 4 INPUT_ATTACHMENT: albedo, normal, materialData, depth
  // =========================================================================
  hina_bind_group_layout_entry gbuffer_entries[4] = {};
  uint32_t gbufferEntryCount = 4;

  // Tile pass: bindings 0-3 are INPUT_ATTACHMENT (3 color + 1 depth from depth_input)
  for (int i = 0; i < 4; ++i) {
    gbuffer_entries[i].binding = static_cast<uint32_t>(i);
    gbuffer_entries[i].type = HINA_DESC_TYPE_INPUT_ATTACHMENT;
    gbuffer_entries[i].count = 1;
    gbuffer_entries[i].stage_flags = HINA_STAGE_FRAGMENT;
    gbuffer_entries[i].flags = HINA_BINDING_FLAG_NONE;
  }

  hina_bind_group_layout_desc gbuffer_layout_desc = {};
  gbuffer_layout_desc.entries = gbuffer_entries;
  gbuffer_layout_desc.entry_count = gbufferEntryCount;
  gbuffer_layout_desc.label = "composite_tile_gbuffer_layout";

  m_compositeLayout.reset(hina_create_bind_group_layout(&gbuffer_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_compositeLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create G-buffer bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // =========================================================================
  // Set 1: Light UBO
  // Layout: vec4 ambient, vec4 cameraPos, mat4 invViewProj, ivec4 lightCounts, vec4 screenInfo, vec4[128] lights
  // Total: 16 + 16 + 64 + 16 + 16 + 2048 = 2176 bytes
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

  // Create light UBO buffer (2176 bytes, host visible for per-frame updates)
  constexpr size_t kLightUBOSize = 16 + 16 + 64 + 16 + 16 + (128 * 16);  // 2176 bytes (added screenInfo)
  hina_buffer_desc ubo_desc = {};
  ubo_desc.size = kLightUBOSize;
  ubo_desc.memory = HINA_BUFFER_CPU;
  ubo_desc.usage = HINA_BUFFER_UNIFORM;
  m_lightUBO = hina_make_buffer(&ubo_desc);
  if (!hina_buffer_is_valid(m_lightUBO)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create light UBO");
    hslc_hsl_module_free(module);
    return false;
  }
  m_lightUBOMapped = hina_mapped_buffer_ptr(m_lightUBO);

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
  // Set 2: Skybox Cubemap
  // =========================================================================
  hina_bind_group_layout_entry skybox_entries[1] = {};
  skybox_entries[0].binding = 0;
  skybox_entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  skybox_entries[0].count = 1;
  skybox_entries[0].stage_flags = HINA_STAGE_FRAGMENT;
  skybox_entries[0].flags = HINA_BINDING_FLAG_NONE;

  hina_bind_group_layout_desc skybox_layout_desc = {};
  skybox_layout_desc.entries = skybox_entries;
  skybox_layout_desc.entry_count = 1;
  skybox_layout_desc.label = "composite_skybox_layout";

  m_skyboxLayout.reset(hina_create_bind_group_layout(&skybox_layout_desc));
  if (!hina_bind_group_layout_is_valid(m_skyboxLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create skybox bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // Create skybox sampler (linear filtering for smooth cubemap sampling)
  {
    gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
    samplerDesc.min_filter = HINA_FILTER_LINEAR;
    samplerDesc.mag_filter = HINA_FILTER_LINEAR;
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_skyboxSampler.reset(gfx::createSampler(samplerDesc));
  }

  // Create fallback 1x1 cubemap (dark blue-gray color) for when no skybox is set
  if (!hina_texture_is_valid(m_fallbackSkyboxTexture)) {
    // Fallback color: RGB(26, 26, 38) = vec4(0.1, 0.1, 0.15, 1.0) from shader
    const uint8_t fallbackColor[4] = { 26, 26, 38, 255 };
    std::vector<uint8_t> cubemapData;
    // 6 faces, each 1x1 pixel, 4 bytes (RGBA)
    for (int face = 0; face < 6; ++face) {
      cubemapData.insert(cubemapData.end(), fallbackColor, fallbackColor + 4);
    }

    hina_texture_desc fallbackDesc = {};
    fallbackDesc.type = HINA_TEX_TYPE_CUBE;
    fallbackDesc.format = HINA_FORMAT_R8G8B8A8_SRGB;
    fallbackDesc.width = 1;
    fallbackDesc.height = 1;
    fallbackDesc.depth = 1;
    fallbackDesc.layers = 6;  // Cubemap has 6 layers
    fallbackDesc.mip_levels = 1;
    fallbackDesc.samples = HINA_SAMPLE_COUNT_1_BIT;
    fallbackDesc.usage = HINA_TEXTURE_SAMPLED_BIT;
    fallbackDesc.initial_data = cubemapData.data();
    fallbackDesc.initial_stride = 0;  // Tight packing - library calculates size from dimensions
    fallbackDesc.label = "fallback_skybox_cubemap";

    m_fallbackSkyboxTexture = hina_make_texture(&fallbackDesc);
    if (hina_texture_is_valid(m_fallbackSkyboxTexture)) {
      m_fallbackSkyboxView = hina_texture_get_default_view(m_fallbackSkyboxTexture);
    } else {
      LOG_WARNING("[SceneRenderFeature] Failed to create fallback skybox cubemap");
    }
  }

  // =========================================================================
  // Create pipeline with all three bind group layouts
  // =========================================================================
  // Tile pass: no vertex input - full-screen triangle from vertex ID
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 0;
  vertex_layout.attr_count = 0;

  // Get tile pass layout for subpass 1 (composite)
  hina_tile_pass_layout tileLayout = GetDeferredTilePassLayout();

  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.depth.depth_test = false;
  pip_desc.depth.depth_write = false;
  pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;
  pip_desc.bind_group_layouts[0] = m_compositeLayout.get();
  pip_desc.bind_group_layouts[1] = m_lightLayout.get();
  pip_desc.bind_group_layouts[2] = m_skyboxLayout.get();

  // Tile pass: subpass 1, with depth read-only for potential depth testing
  pip_desc.depth_format = HINA_FORMAT_D32_SFLOAT;
  pip_desc.tile_layout = &tileLayout;
  pip_desc.subpass_index = 1;
  LOG_INFO("[SceneRenderFeature] Creating tile-based composite pipeline (subpass 1)");

  m_compositePipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_compositePipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Composite pipeline creation failed");
    return false;
  }

  m_compositePipelineCreated = true;
  LOG_INFO("[SceneRenderFeature] Composite pipeline created (tile_pass)");
  return true;
}

bool SceneRenderFeature::EnsureCompositeBindGroup(GfxRenderer* gfxRenderer, gfx::Texture sceneDepth, gfx::TextureView sceneDepthView)
{
  if (!gfxRenderer) return false;

  const auto& gbuffer = gfxRenderer->getGBuffer();

  // Check if G-buffer dimensions changed (resize occurred)
  // In tile pass mode, we don't use depth texture so don't check sceneDepth.id
  uint32_t gbufferWidth = 0, gbufferHeight = 0;
  hina_get_texture_size(gbuffer.albedo, &gbufferWidth, &gbufferHeight);

  bool needsRecreate = !hina_bind_group_is_valid(m_compositeBindGroup)
                    || gbufferWidth != m_lastGBufferWidth
                    || gbufferHeight != m_lastGBufferHeight;

  if (!needsRecreate) {
    return true;  // Existing bind group is still valid
  }

  // Destroy old bind group if it exists
  if (hina_bind_group_is_valid(m_compositeBindGroup)) {
    hina_destroy_bind_group(m_compositeBindGroup);
    m_compositeBindGroup = {};
  }

  // Create new bind group with current G-buffer textures
  // Tile pass: bindings 0-3 are INPUT_ATTACHMENT (3 color + 1 depth)
  hina_bind_group_entry entries[4] = {};

  entries[0].binding = 0;  // albedo (tile input)
  entries[0].type = HINA_DESC_TYPE_INPUT_ATTACHMENT;
  entries[0].view = gbuffer.albedoView;

  entries[1].binding = 1;  // normal (tile input)
  entries[1].type = HINA_DESC_TYPE_INPUT_ATTACHMENT;
  entries[1].view = gbuffer.normalView;

  entries[2].binding = 2;  // materialData (tile input)
  entries[2].type = HINA_DESC_TYPE_INPUT_ATTACHMENT;
  entries[2].view = gbuffer.materialDataView;

  entries[3].binding = 3;  // depth (tile input from D32_SFLOAT)
  entries[3].type = HINA_DESC_TYPE_INPUT_ATTACHMENT;
  entries[3].view = sceneDepthView;

  uint32_t entryCount = 4;

  hina_bind_group_desc bg_desc = {};
  bg_desc.layout = m_compositeLayout.get();
  bg_desc.entries = entries;
  bg_desc.entry_count = entryCount;
  bg_desc.label = "scene_composite_tile_bg";

  m_compositeBindGroup = hina_create_bind_group(&bg_desc);
  if (!hina_bind_group_is_valid(m_compositeBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create composite bind group");
    return false;
  }

  m_lastGBufferWidth = gbufferWidth;
  m_lastGBufferHeight = gbufferHeight;
  return true;
}

bool SceneRenderFeature::EnsureSkyboxBindGroup(GfxRenderer* gfxRenderer)
{
  if (!gfxRenderer) return false;

  // Get skybox texture from renderer
  gfx::Texture skyboxTexture = gfxRenderer->getSkyboxTexture();
  gfx::TextureView skyboxView = gfxRenderer->getSkyboxTextureView();

  // Use fallback if no skybox is set
  bool usingFallback = false;
  if (!hina_texture_is_valid(skyboxTexture) || !hina_texture_view_is_valid(skyboxView)) {
    // Use fallback cubemap
    if (!hina_texture_is_valid(m_fallbackSkyboxTexture)) {
      LOG_WARNING("[SceneRenderFeature] No skybox and no fallback available");
      return false;
    }
    skyboxTexture = m_fallbackSkyboxTexture;
    skyboxView = m_fallbackSkyboxView;
    usingFallback = true;
  }

  // Check if we need to recreate the bind group
  bool needsRecreate = !hina_bind_group_is_valid(m_skyboxBindGroup)
                    || skyboxTexture.id != m_lastSkyboxTexture.id;

  if (!needsRecreate) {
    return true;  // Existing bind group is still valid
  }

  // Destroy old bind group if it exists
  if (hina_bind_group_is_valid(m_skyboxBindGroup)) {
    hina_destroy_bind_group(m_skyboxBindGroup);
    m_skyboxBindGroup = {};
  }

  // Create skybox bind group
  hina_bind_group_entry entries[1] = {};
  entries[0].binding = 0;
  entries[0].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
  entries[0].combined.view = skyboxView;
  entries[0].combined.sampler = m_skyboxSampler.get();

  hina_bind_group_desc bg_desc = {};
  bg_desc.layout = m_skyboxLayout.get();
  bg_desc.entries = entries;
  bg_desc.entry_count = 1;
  bg_desc.label = usingFallback ? "scene_skybox_fallback_bg" : "scene_skybox_bg";

  m_skyboxBindGroup = hina_create_bind_group(&bg_desc);
  if (!hina_bind_group_is_valid(m_skyboxBindGroup)) {
    LOG_ERROR("[SceneRenderFeature] Failed to create skybox bind group");
    return false;
  }

  m_lastSkyboxTexture = skyboxTexture;
  m_skyboxBindGroupCreated = true;
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
  //   vec4 screenInfo      (16 bytes, offset 112) - xy = screenSize, zw = 1/screenSize
  //   vec4 lights[128]     (2048 bytes, offset 128)
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

    // screenInfo (offset 112) - for mobile tile-based rendering position reconstruction
    // Using gl_FragCoord * invScreenSize instead of interpolated UV avoids precision
    // issues at tile boundaries on mobile GPUs
    float screenWidth = static_cast<float>(RenderResources::INTERNAL_WIDTH);
    float screenHeight = static_cast<float>(RenderResources::INTERNAL_HEIGHT);
    glm::vec4 screenInfo(screenWidth, screenHeight, 1.0f / screenWidth, 1.0f / screenHeight);
    std::memcpy(uboData + offset, &screenInfo, sizeof(glm::vec4));
    offset += 16;

    // lights array (offset 128)
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

  // Bind bind groups: set 0 = G-buffer textures, set 1 = light UBO, set 2 = skybox (if available)
  hina_cmd_bind_group(cmd, 0, m_compositeBindGroup);
  hina_cmd_bind_group(cmd, 1, m_lightBindGroup);

  // Try to bind skybox (set 2) - if unavailable, shader falls back to solid color
  if (EnsureSkyboxBindGroup(gfxRenderer)) {
    hina_cmd_bind_group(cmd, 2, m_skyboxBindGroup);
  }

  // Tile pass: full-screen triangle from vertex ID (no vertex buffer)
  hina_cmd_draw(cmd, 3, 1, 0, 0);
}

// =============================================================================
// ExecuteDeferredTilePass - Combined G-buffer + Composite as single tile pass
// =============================================================================

void SceneRenderFeature::ExecuteDeferredTilePass(internal::ExecutionContext& ctx)
{
  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) return;

  // Ensure all pipelines are created
  if (!EnsurePipelineCreated(gfxRenderer)) return;
  if (!EnsureCompositePipelineCreated(gfxRenderer)) return;

  const FrameData& frameData = ctx.GetFrameData();
  assert(frameData.projMatrix != glm::mat4(1.0f) && "projMatrix is identity!");
  assert(frameData.viewMatrix != glm::mat4(1.0f) && "viewMatrix is identity!");

  gfx::Cmd* cmd = ctx.GetCmd();
  const auto& gbuffer = gfxRenderer->getGBuffer();
  gfx::Texture sceneColor = ctx.GetTexture(RenderResources::SCENE_COLOR);
  gfx::Texture sceneDepth = ctx.GetTexture(RenderResources::SCENE_DEPTH);
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);
  gfx::TextureView sceneColorView = hina_texture_get_default_view(sceneColor);

  // =========================================================================
  // Setup tile pass descriptor with 2 subpasses
  // =========================================================================
  hina_tile_pass_desc tilePass = {};
  tilePass.label = "deferred_tile_pass";

  // Subpass 0: G-Buffer fill (4 color outputs + depth)
  tilePass.subpasses[0].label = "gbuffer";
  tilePass.subpasses[0].color_count = 4;

  tilePass.subpasses[0].color[0].image = gbuffer.albedoView;
  tilePass.subpasses[0].color[0].load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[0].color[0].store_op = HINA_STORE_OP_STORE;

  tilePass.subpasses[0].color[1].image = gbuffer.normalView;
  tilePass.subpasses[0].color[1].load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[0].color[1].store_op = HINA_STORE_OP_STORE;
  tilePass.subpasses[0].color[1].clear_color[0] = 0.5f;
  tilePass.subpasses[0].color[1].clear_color[1] = 0.5f;

  tilePass.subpasses[0].color[2].image = gbuffer.materialDataView;
  tilePass.subpasses[0].color[2].load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[0].color[2].store_op = HINA_STORE_OP_STORE;

  tilePass.subpasses[0].color[3].image = gbuffer.visibilityIDView;
  tilePass.subpasses[0].color[3].load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[0].color[3].store_op = HINA_STORE_OP_STORE;

  tilePass.subpasses[0].has_depth = true;
  tilePass.subpasses[0].depth.image = sceneDepthView;
  tilePass.subpasses[0].depth.load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[0].depth.store_op = HINA_STORE_OP_STORE;
  tilePass.subpasses[0].depth.depth_clear = 1.0f;

  // Subpass 1: Composition (HDR output, tile inputs from subpass 0)
  tilePass.subpasses[1].label = "composite";
  tilePass.subpasses[1].color_count = 1;
  tilePass.subpasses[1].color[0].image = sceneColorView;
  tilePass.subpasses[1].color[0].load_op = HINA_LOAD_OP_CLEAR;
  tilePass.subpasses[1].color[0].store_op = HINA_STORE_OP_STORE;
  tilePass.subpasses[1].color[0].clear_color[0] = 0.1f;
  tilePass.subpasses[1].color[0].clear_color[1] = 0.1f;
  tilePass.subpasses[1].color[0].clear_color[2] = 0.15f;
  tilePass.subpasses[1].color[0].clear_color[3] = 1.0f;

  // Tile inputs from subpass 0 (albedo, normal, materialData)
  tilePass.subpasses[1].tile_input_count = 3;
  tilePass.subpasses[1].tile_inputs[0] = {0, 0};  // albedo
  tilePass.subpasses[1].tile_inputs[1] = {0, 1};  // normal
  tilePass.subpasses[1].tile_inputs[2] = {0, 2};  // materialData

  // Depth for depth testing (preserve from subpass 0) + read as tile input
  tilePass.subpasses[1].has_depth = true;
  tilePass.subpasses[1].depth.image = sceneDepthView;
  tilePass.subpasses[1].depth.load_op = HINA_LOAD_OP_LOAD;
  tilePass.subpasses[1].depth.store_op = HINA_STORE_OP_STORE;
  tilePass.subpasses[1].depth_read_only = true;
  tilePass.subpasses[1].depth_input = true;  // Read depth as tile input for world pos reconstruction

  // =========================================================================
  // Begin tile pass
  // =========================================================================
  if (!hina_begin_tile_pass(cmd, &tilePass)) {
    LOG_ERROR("[SceneRenderFeature] Failed to begin tile pass");
    return;
  }

  // =========================================================================
  // SUBPASS 0: G-Buffer fill
  // =========================================================================
  glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  std::memcpy(m_frameUBOMapped, &viewProj, sizeof(glm::mat4));

  hina_cmd_bind_pipeline(cmd, m_gbufferPipeline.get());
  hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

  // Draw submitted meshes (same logic as ExecuteGBufferPass)
  if (!m_drawList.empty()) {
    auto& meshStorage = gfxRenderer->getMeshStorage();
    auto& materialSystem = gfxRenderer->getMaterialSystem();
    auto& frame = gfxRenderer->getCurrentFrameResources();

    if (!frame.transformRingMapped) {
      hina_end_tile_pass(cmd);
      return;
    }

    // Separate draws into static, skinned, and morphed
    std::vector<const DrawData*> staticDraws;
    std::vector<const DrawData*> skinnedDraws;
    std::vector<const DrawData*> morphedDraws;
    staticDraws.reserve(m_drawList.size());
    skinnedDraws.reserve(32);
    morphedDraws.reserve(16);

    for (const auto& draw : m_drawList) {
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      bool meshHasSkinning = gpuMesh && gpuMesh->valid && gpuMesh->hasSkinning;
      bool meshHasMorphs = gpuMesh && gpuMesh->valid && gpuMesh->hasMorphs;

      if (draw.isSkinned && draw.skinMatrices && meshHasSkinning) {
        if (draw.hasMorphs && draw.morphWeights && meshHasMorphs) {
          morphedDraws.push_back(&draw);
        } else {
          skinnedDraws.push_back(&draw);
        }
      } else {
        staticDraws.push_back(&draw);
      }
    }

    // Sort by material
    auto sortByMaterial = [](const DrawData* a, const DrawData* b) {
      if (a->gfxMaterial.index != b->gfxMaterial.index)
        return a->gfxMaterial.index < b->gfxMaterial.index;
      return a->gfxMaterial.generation < b->gfxMaterial.generation;
    };
    std::sort(staticDraws.begin(), staticDraws.end(), sortByMaterial);
    std::sort(skinnedDraws.begin(), skinnedDraws.end(), sortByMaterial);
    std::sort(morphedDraws.begin(), morphedDraws.end(), sortByMaterial);

    // Draw static meshes
    gfx::BindGroup lastMaterialBG = {};
    for (const DrawData* drawPtr : staticDraws) {
      const DrawData& draw = *drawPtr;
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      if (!gpuMesh || !gpuMesh->valid) continue;
      if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) break;

      uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
      glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
      *transformDst = draw.transform;
      uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
      *objectIdDst = draw.objectId;
      // Write flags at offset 68 - bit 0 = negative determinant
      uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
      float det = glm::determinant(draw.transform);
      *flagsDst = (det < 0.0f) ? 1u : 0u;

      hina_vertex_input meshInput = {};
      meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
      meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
      meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
      meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
      meshInput.index_buffer = gpuMesh->indexBuffer;
      meshInput.index_type = HINA_INDEX_UINT32;
      hina_cmd_apply_vertex_input(cmd, &meshInput);

      gfx::BindGroup materialBG;
      if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
        materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
      } else {
        materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
      }
      if (materialBG.id != lastMaterialBG.id) {
        hina_cmd_bind_group(cmd, 1, materialBG);
        lastMaterialBG = materialBG;
      }

      uint32_t dynamicOffsets[2] = { frame.transformRingOffset, 0 };
      hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);
      frame.transformRingOffset += TRANSFORM_ALIGNMENT;

      hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
    }

    // Draw skinned meshes
    if (!skinnedDraws.empty() && EnsureSkinnedPipelineCreated(gfxRenderer) && frame.boneMatrixRingMapped) {
      lastMaterialBG = {};
      hina_cmd_bind_pipeline(cmd, m_gbufferSkinnedPipeline.get());
      hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

      for (const DrawData* drawPtr : skinnedDraws) {
        const DrawData& draw = *drawPtr;
        const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
        if (!gpuMesh || !gpuMesh->valid) continue;
        if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE || frame.boneMatrixRingOffset >= BONE_MATRIX_UBO_SIZE) break;

        uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
        glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
        *transformDst = draw.transform;
        uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
        *objectIdDst = draw.objectId;
        // Write flags at offset 68 - bit 0 = negative determinant
        uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
        float det = glm::determinant(draw.transform);
        *flagsDst = (det < 0.0f) ? 1u : 0u;

        uint32_t numBones = std::min(draw.jointCount, gfx::MAX_BONES_PER_MESH);
        glm::mat4* bonesDst = reinterpret_cast<glm::mat4*>(
            static_cast<uint8_t*>(frame.boneMatrixRingMapped) + frame.boneMatrixRingOffset);
        static const glm::mat4 identity(1.0f);
        for (uint32_t i = 0; i < gfx::MAX_BONES_PER_MESH; ++i) bonesDst[i] = identity;
        std::memcpy(bonesDst, draw.skinMatrices, numBones * sizeof(glm::mat4));

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

        gfx::BindGroup materialBG;
        if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
          materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
        } else {
          materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
        }
        if (materialBG.id != lastMaterialBG.id) {
          hina_cmd_bind_group(cmd, 1, materialBG);
          lastMaterialBG = materialBG;
        }

        uint32_t dynamicOffsets[2] = { frame.transformRingOffset, frame.boneMatrixRingOffset };
        hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);
        frame.transformRingOffset += TRANSFORM_ALIGNMENT;
        frame.boneMatrixRingOffset += gfx::BONE_MATRICES_SIZE;

        hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
      }
    }

    // Draw morphed meshes (using skinned pipeline for now)
    if (!morphedDraws.empty() && EnsureSkinnedPipelineCreated(gfxRenderer) && frame.boneMatrixRingMapped) {
      lastMaterialBG = {};
      hina_cmd_bind_pipeline(cmd, m_gbufferSkinnedPipeline.get());
      hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

      for (const DrawData* drawPtr : morphedDraws) {
        const DrawData& draw = *drawPtr;
        const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
        if (!gpuMesh || !gpuMesh->valid) continue;
        if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE || frame.boneMatrixRingOffset >= BONE_MATRIX_UBO_SIZE) break;

        uint8_t* slotPtr = static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset;
        glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(slotPtr);
        *transformDst = draw.transform;
        uint32_t* objectIdDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4));
        *objectIdDst = draw.objectId;
        // Write flags at offset 68 - bit 0 = negative determinant
        uint32_t* flagsDst = reinterpret_cast<uint32_t*>(slotPtr + sizeof(glm::mat4) + sizeof(uint32_t));
        float det = glm::determinant(draw.transform);
        *flagsDst = (det < 0.0f) ? 1u : 0u;

        uint32_t numBones = std::min(draw.jointCount, gfx::MAX_BONES_PER_MESH);
        glm::mat4* bonesDst = reinterpret_cast<glm::mat4*>(
            static_cast<uint8_t*>(frame.boneMatrixRingMapped) + frame.boneMatrixRingOffset);
        static const glm::mat4 identity(1.0f);
        for (uint32_t i = 0; i < gfx::MAX_BONES_PER_MESH; ++i) bonesDst[i] = identity;
        std::memcpy(bonesDst, draw.skinMatrices, numBones * sizeof(glm::mat4));

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

        gfx::BindGroup materialBG;
        if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
          materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
        } else {
          materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
        }
        if (materialBG.id != lastMaterialBG.id) {
          hina_cmd_bind_group(cmd, 1, materialBG);
          lastMaterialBG = materialBG;
        }

        uint32_t dynamicOffsets[2] = { frame.transformRingOffset, frame.boneMatrixRingOffset };
        hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 2);
        frame.transformRingOffset += TRANSFORM_ALIGNMENT;
        frame.boneMatrixRingOffset += gfx::BONE_MATRICES_SIZE;

        hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
      }
    }
  }

  // =========================================================================
  // Transition to SUBPASS 1: Composition
  // =========================================================================
  hina_tile_pass_next(cmd, &tilePass);

  // Ensure composite bind group is valid
  if (!EnsureCompositeBindGroup(gfxRenderer, sceneDepth, sceneDepthView)) {
    hina_end_tile_pass(cmd);
    return;
  }

  // Fill light UBO (same as ExecuteCompositePass)
  if (m_lightUBOMapped) {
    uint8_t* uboData = static_cast<uint8_t*>(m_lightUBOMapped);
    size_t offset = 0;

    glm::vec4 ambient(0.15f, 0.17f, 0.2f, 1.0f);
    std::memcpy(uboData + offset, &ambient, sizeof(glm::vec4));
    offset += 16;

    glm::vec4 cameraPos(frameData.cameraPos, 1.0f);
    std::memcpy(uboData + offset, &cameraPos, sizeof(glm::vec4));
    offset += 16;

    glm::mat4 invViewProj = glm::inverse(viewProj);
    std::memcpy(uboData + offset, &invViewProj, sizeof(glm::mat4));
    offset += 64;

    int numLights = static_cast<int>(std::min(m_lightList.size(), size_t(32)));
    glm::ivec4 lightCounts(numLights, 0, 0, 0);
    std::memcpy(uboData + offset, &lightCounts, sizeof(glm::ivec4));
    offset += 16;

    // screenInfo - for mobile tile-based rendering position reconstruction
    float screenWidth = static_cast<float>(RenderResources::INTERNAL_WIDTH);
    float screenHeight = static_cast<float>(RenderResources::INTERNAL_HEIGHT);
    glm::vec4 screenInfo(screenWidth, screenHeight, 1.0f / screenWidth, 1.0f / screenHeight);
    std::memcpy(uboData + offset, &screenInfo, sizeof(glm::vec4));
    offset += 16;

    for (int i = 0; i < numLights; ++i) {
      const CollectedLight& light = m_lightList[i];

      glm::vec4 data0(static_cast<float>(light.type), light.position.x, light.position.y, light.position.z);
      std::memcpy(uboData + offset, &data0, sizeof(glm::vec4));
      offset += 16;

      glm::vec4 data1(light.direction.x, light.direction.y, light.direction.z, light.intensity);
      std::memcpy(uboData + offset, &data1, sizeof(glm::vec4));
      offset += 16;

      glm::vec4 data2(light.color.x, light.color.y, light.color.z, light.radius);
      std::memcpy(uboData + offset, &data2, sizeof(glm::vec4));
      offset += 16;

      glm::vec4 data3(light.attenuation.x, light.attenuation.y, light.attenuation.z, 0.0f);
      std::memcpy(uboData + offset, &data3, sizeof(glm::vec4));
      offset += 16;
    }
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

  hina_cmd_bind_pipeline(cmd, m_compositePipeline.get());
  hina_cmd_bind_group(cmd, 0, m_compositeBindGroup);
  hina_cmd_bind_group(cmd, 1, m_lightBindGroup);

  if (EnsureSkyboxBindGroup(gfxRenderer)) {
    hina_cmd_bind_group(cmd, 2, m_skyboxBindGroup);
  }

  // Draw full-screen triangle (tile pass uses vertex ID)
  hina_cmd_draw(cmd, 3, 1, 0, 0);

  // =========================================================================
  // End tile pass
  // =========================================================================
  hina_end_tile_pass(cmd);
}
