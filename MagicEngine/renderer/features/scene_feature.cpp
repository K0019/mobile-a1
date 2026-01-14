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
#include "renderer/renderer.h"
#include "renderer/render_graph.h"
#include "renderer/linear_color.h"
#include "resource/resource_manager.h"
#include "resource/resource_types.h"
#include "Graphics/RenderComponent.h"
#include "ECS/ECS.h"
#include "logging/log.h"
#include <cstring>

// =============================================================================
// G-Buffer Shader (embedded hina_sl)
// =============================================================================

// G-buffer shader - exact copy from GfxRenderer to verify compilation
static const char* kGBufferShader = R"(#hina
group Frame = 0;
group Material = 1;

bindings(Frame, start=0) {
  uniform(std140) FrameUBO {
    mat4 viewProj;
  } frame;
}

bindings(Material, start=1) {
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_metallicRoughness;
  texture sampler2D u_emissive;
  texture sampler2D u_occlusion;
}

push_constant PushConstants {
  mat4 model;
  vec4 baseColor;
} pc;

// Split vertex streams (24 bytes total):
// Buffer 0: Position stream (12 bytes)
// Buffer 1: Attribute stream (12 bytes packed)
struct VertexIn {
  vec3 a_position;    // location 0, buffer 0: position
  uint a_normalMatID; // location 1, buffer 1: RGB10A2 octahedral normal + 2-bit matID
  uint a_tangentSign; // location 2, buffer 1: RGB10A2 octahedral tangent + sign
  uint a_uv;          // location 3, buffer 1: RG16_UNORM UV
};

struct Varyings {
  vec3 worldPos;
  vec3 normal;
  vec3 tangent;
  float bitangentSign;
  vec2 uv;
};

struct FragOut {
  vec4 albedo;       // location 0
  vec4 normal;       // location 1
  vec4 materialData; // location 2
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;

    // Position from stream 0
    vec4 worldPos = pc.model * vec4(in.a_position, 1.0);
    out.worldPos = worldPos.xyz;

    // Inline octahedral decode helper
    // Unpack normal from RGB10A2 octahedral encoding
    float nx = float(in.a_normalMatID & 0x3FFu) / 1023.0;
    float ny = float((in.a_normalMatID >> 10u) & 0x3FFu) / 1023.0;
    vec2 nf = vec2(nx, ny) * 2.0 - 1.0;
    vec3 localNormal = vec3(nf.xy, 1.0 - abs(nf.x) - abs(nf.y));
    float nt = max(-localNormal.z, 0.0);
    localNormal.x += (localNormal.x >= 0.0) ? -nt : nt;
    localNormal.y += (localNormal.y >= 0.0) ? -nt : nt;
    localNormal = normalize(localNormal);

    // Unpack tangent from RGB10A2 octahedral encoding
    float tx = float(in.a_tangentSign & 0x3FFu) / 1023.0;
    float ty = float((in.a_tangentSign >> 10u) & 0x3FFu) / 1023.0;
    vec2 tf = vec2(tx, ty) * 2.0 - 1.0;
    vec3 localTangent = vec3(tf.xy, 1.0 - abs(tf.x) - abs(tf.y));
    float tt = max(-localTangent.z, 0.0);
    localTangent.x += (localTangent.x >= 0.0) ? -tt : tt;
    localTangent.y += (localTangent.y >= 0.0) ? -tt : tt;
    localTangent = normalize(localTangent);

    // Extract sign bit for bitangent
    uint signBit = (in.a_tangentSign >> 30u) & 0x3u;

    mat3 normalMatrix = mat3(pc.model);
    out.normal = normalMatrix * localNormal;
    out.tangent = normalMatrix * localTangent;
    out.bitangentSign = (signBit & 1u) != 0u ? -1.0 : 1.0;

    // Unpack UV from RG16_UNORM
    out.uv = unpackUnorm2x16(in.a_uv);

    gl_Position = frame.viewProj * worldPos;
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // Sample albedo texture
    vec4 albedo = texture(u_albedo, in.uv) * pc.baseColor;

    // Sample normal map and construct TBN
    vec3 N = normalize(in.normal);
    vec3 T = normalize(in.tangent);
    vec3 B = cross(N, T) * in.bitangentSign;
    mat3 TBN = mat3(T, B, N);

    vec3 normalMap = texture(u_normal, in.uv).rgb * 2.0 - 1.0;
    N = normalize(TBN * normalMap);

    // Sample metallic/roughness (B=metallic, G=roughness in glTF convention)
    vec2 mr = texture(u_metallicRoughness, in.uv).bg;
    float metallic = mr.x;
    float roughness = mr.y;

    // Sample occlusion (R channel)
    float ao = texture(u_occlusion, in.uv).r;

    // Output albedo from material texture
    out.albedo = albedo;

    // Encode world-space normal to [0,1] range for storage
    out.normal = vec4(N.xy * 0.5 + 0.5, 0.0, 1.0);

    // Material data: roughness, metallic, ao, emissive intensity
    out.materialData = vec4(roughness, metallic, ao, 0.0);

    return out;
}
#hina_end
)";

// =============================================================================
// Composite Shader (embedded hina_sl) - Deferred lighting pass
// =============================================================================

static const char* kCompositeShader = R"(#hina
group Composite = 0;

bindings(Composite, start=0) {
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_materialData;
  texture sampler2D u_depth;
}

push_constant LightingParams {
  vec4 lightDir;
  vec4 lightColor;
  vec4 ambientColor;
  vec4 cameraPos;
  mat4 invViewProj;
} pc;

struct VertexIn {
  vec2 a_position;
  vec2 a_uv;
};

struct Varyings {
  vec2 uv;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain(VertexIn in) {
    Varyings out;
    out.uv = in.a_uv;
    gl_Position = vec4(in.a_position, 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // G-buffer is Y-flipped due to Vulkan projection flip - flip UV for all G-buffer samples
    vec2 gbufferUV = vec2(in.uv.x, in.uv.y);

    // Sample G-buffer with flipped UV
    vec4 albedoSample = texture(u_albedo, gbufferUV);
    vec2 encodedNormal = texture(u_normal, gbufferUV).xy;
    vec4 matData = texture(u_materialData, gbufferUV);
    float depth = texture(u_depth, gbufferUV).r;

    // Early out for sky/background (depth == 1.0)
    if (depth >= 1.0) {
        out.color = vec4(0.1, 0.1, 0.15, 1.0);  // Sky color
        return out;
    }

    // Decode normal from octahedral (simple encoding for now)
    vec3 normal = vec3(encodedNormal * 2.0 - 1.0, 0.0);
    normal.z = sqrt(max(0.0, 1.0 - dot(normal.xy, normal.xy)));
    normal = normalize(normal);

    // Extract material properties
    float roughness = matData.r;
    float metallic = matData.g;

    // Reconstruct world position from depth
    // Note: projMatrix already has Vulkan Y-flip applied, so no additional flip needed here
    vec2 ndc = gbufferUV * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 worldPos4 = pc.invViewProj * clipPos;
    vec3 worldPos = worldPos4.xyz / worldPos4.w;

    // View direction
    vec3 viewDir = normalize(pc.cameraPos.xyz - worldPos);

    // Directional light
    vec3 lightDir = normalize(pc.lightDir.xyz);
    float lightIntensity = pc.lightDir.w;

    // Diffuse lighting (Lambert)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = pc.lightColor.rgb * NdotL * lightIntensity;

    // Simple specular (Blinn-Phong)
    vec3 halfVec = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfVec), 0.0);
    float specPower = mix(8.0, 128.0, 1.0 - roughness);
    float spec = pow(NdotH, specPower) * (1.0 - roughness) * 0.5;
    vec3 specular = pc.lightColor.rgb * spec * lightIntensity;

    // Combine lighting
    vec3 ambient = pc.ambientColor.rgb;
    vec3 baseColor = albedoSample.rgb;  // Use albedo from G-buffer

    vec3 finalColor = baseColor * (ambient + diffuse) + specular;

    // Output HDR color - tone mapping handled by separate pass
    out.color = vec4(finalColor, 1.0);

    return out;
}
#hina_end
)";

// =============================================================================
// SceneRenderFeature Implementation
// =============================================================================

SceneRenderFeature::SceneRenderFeature(bool enableObjectPicking)
  : m_enableObjectPicking(enableObjectPicking)
{
  LOG_INFO("[SceneRenderFeature] Created");
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
  m_sceneLayout.reset();
  m_pipelineCreated = false;

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

  m_drawList.clear();
}

bool SceneRenderFeature::EnsurePipelineCreated(GfxRenderer* gfxRenderer)
{
  if (m_pipelineCreated) {
    return true;
  }

  if (!gfxRenderer) {
    return false;
  }

  LOG_INFO("[SceneRenderFeature] Creating G-buffer pipeline...");

  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kGBufferShader, "scene_gbuffer", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // Verify we got valid SPIR-V data
  LOG_INFO("[SceneRenderFeature] Module compiled - VS spirv: {} bytes, FS spirv: {} bytes",
           module->vs.spirv_size, module->fs.spirv_size);
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
  // Check SPIR-V magic number
  const uint32_t* vs_spirv = (const uint32_t*)module->vs.spirv_data;
  const uint32_t* fs_spirv = (const uint32_t*)module->fs.spirv_data;
  LOG_INFO("[SceneRenderFeature] VS SPIR-V magic: 0x{:08X}, FS SPIR-V magic: 0x{:08X}",
           vs_spirv[0], fs_spirv[0]);
  if (vs_spirv[0] != 0x07230203) {
    LOG_ERROR("[SceneRenderFeature] VS SPIR-V has invalid magic number!");
  }
  if (fs_spirv[0] != 0x07230203) {
    LOG_ERROR("[SceneRenderFeature] FS SPIR-V has invalid magic number!");
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

  // Material layout from GfxRenderer
  gfx::BindGroupLayout materialLayout = gfxRenderer->getMaterialLayout();

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
  pip_desc.cull_mode = HINA_CULL_MODE_BACK;
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
  pip_desc.bind_group_layout_count = 2;

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
  LOG_INFO("[SceneRenderFeature] G-buffer pipeline and resources created");
  return true;
}

void SceneRenderFeature::UpdateScene(uint64_t renderFeatureID,
                                      const Resource::ResourceManager& resourceMngr,
                                      Renderer& renderer)
{
  auto* feature = renderer.GetFeature<SceneRenderFeature>(renderFeatureID);
  if (!feature) {
    return;
  }

  feature->m_drawList.clear();

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

    const auto& materialList = comp.GetMaterialsList();
    const size_t materialCount = materialList.size();

    for (size_t i = 0; i < mesh->handles.size(); ++i)
    {
      const MeshHandle& meshHandle = mesh->handles[i];
      if (!meshHandle.isValid()) continue;

      const MeshGPUData* meshData = resourceMngr.getMesh(meshHandle);
      if (!meshData || !meshData->hasGfxMesh) continue;

      DrawData draw;
      draw.gfxMesh.index = static_cast<uint16_t>(meshData->gfxMeshIndex);
      draw.gfxMesh.generation = static_cast<uint16_t>(meshData->gfxMeshGeneration);
      draw.transform = entityWorldMatrix * mesh->transforms[i];
      draw.baseColor = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);

      const ResourceMaterial* material = nullptr;
      if (i < materialCount) {
        material = materialList[i].GetResource();
      }
      if (!material) {
        material = MagicResourceManager::GetContainer<ResourceMaterial>().GetResource(
            mesh->defaultMaterialHashes[i]);
      }

      if (material && material->handle.isValid()) {
        const auto* matHotData = resourceMngr.getMaterialHotData(material->handle);
        if (matHotData && matHotData->hasGfxMaterial) {
          draw.gfxMaterial.index = static_cast<uint16_t>(matHotData->gfxMaterialIndex);
          draw.gfxMaterial.generation = static_cast<uint16_t>(matHotData->gfxMaterialGeneration);
          draw.hasMaterial = true;
        }
      }

      feature->m_drawList.push_back(draw);
    }
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

  LOG_INFO("[SceneRenderFeature] SetupPasses - registered G-buffer and composite passes");
}

void SceneRenderFeature::ExecuteGBufferPass(internal::ExecutionContext& ctx)
{
  GfxRenderer* gfxRenderer = ctx.GetGfxRenderer();
  if (!gfxRenderer) return;

  // Ensure pipeline and persistent resources are created
  if (!EnsurePipelineCreated(gfxRenderer)) return;

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
  const FrameData& frameData = ctx.GetFrameData();

  // projMatrix already has Vulkan Y-flip applied at source (UploadToPipeline)
  glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  std::memcpy(m_frameUBOMapped, &viewProj, sizeof(glm::mat4));

  // Bind pipeline and persistent frame bind group
  hina_cmd_bind_pipeline(cmd, m_gbufferPipeline.get());
  hina_cmd_bind_group(cmd, 0, m_sceneBindGroup);

  // Draw submitted meshes
  if (!m_drawList.empty()) {
    struct { glm::mat4 model; glm::vec4 baseColor; } pushConstants;

    auto& meshStorage = gfxRenderer->getMeshStorage();
    auto& materialSystem = gfxRenderer->getMaterialSystem();

    for (const auto& draw : m_drawList) {
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      if (!gpuMesh || !gpuMesh->valid) continue;

      // Bind split vertex streams with offsets
      hina_vertex_input meshInput = {};
      meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
      meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
      meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
      meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
      meshInput.index_buffer = gpuMesh->indexBuffer;
      meshInput.index_type = HINA_INDEX_UINT32;

      hina_cmd_apply_vertex_input(cmd, &meshInput);

      // Bind material at Set 1 and get baseColor from material
      gfx::BindGroup materialBG;
      glm::vec4 baseColor = draw.baseColor;  // Default fallback
      if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
        materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
        // Get actual baseColor from the material
        const gfx::MaterialEntry* matEntry = materialSystem.getMaterial(draw.gfxMaterial);
        if (matEntry) {
          baseColor = matEntry->material.baseColor;
        }
      } else {
        materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
      }
      hina_cmd_bind_group(cmd, 1, materialBG);

      // Update push constants for this draw
      pushConstants.model = draw.transform;
      pushConstants.baseColor = baseColor;  // Use material's baseColor if available
      hina_cmd_push_constants(cmd, 0, sizeof(pushConstants), &pushConstants);

      // Draw with index offset
      hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
    }
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
void SceneRenderFeature::ExecuteWBOITAccumulation(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteWBOITResolve(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ProcessPendingPick(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::ExecuteLightingSetup(internal::ExecutionContext& ctx) { (void)ctx; }

void SceneRenderFeature::EnsureDeformationPipeline(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureGBufferPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureSSAOPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureShadowPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureDeferredLightingPipeline(internal::ExecutionContext& ctx) { (void)ctx; }
void SceneRenderFeature::EnsureWBOITPipelines(internal::ExecutionContext& ctx) { (void)ctx; }
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

  LOG_INFO("[SceneRenderFeature] Creating composite pipeline...");

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
    // Fullscreen quad vertices: pos(2) + uv(2) = 16 bytes per vertex
    const float quadVerts[] = {
      // Triangle 1
      -1.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
       1.0f, -1.0f, 1.0f, 0.0f,  // bottom-right
       1.0f,  1.0f, 1.0f, 1.0f,  // top-right
      // Triangle 2
      -1.0f, -1.0f, 0.0f, 0.0f,  // bottom-left
       1.0f,  1.0f, 1.0f, 1.0f,  // top-right
      -1.0f,  1.0f, 0.0f, 1.0f   // top-left
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

  // Compile composite shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kCompositeShader, "scene_composite", &error);
  if (!module) {
    LOG_ERROR("[SceneRenderFeature] Composite shader compilation failed: {}", error ? error : "unknown");
    if (error) hslc_free_log(error);
    return false;
  }

  // Create bind group layout for G-buffer textures (set 0: 4 combined image samplers)
  hina_bind_group_layout_entry layout_entries[4] = {};
  for (int i = 0; i < 4; ++i) {
    layout_entries[i].binding = static_cast<uint32_t>(i);
    layout_entries[i].type = HINA_DESC_TYPE_COMBINED_IMAGE_SAMPLER;
    layout_entries[i].count = 1;
    layout_entries[i].stage_flags = HINA_STAGE_FRAGMENT;
    layout_entries[i].flags = HINA_BINDING_FLAG_NONE;
  }

  hina_bind_group_layout_desc layout_desc = {};
  layout_desc.entries = layout_entries;
  layout_desc.entry_count = 4;
  layout_desc.label = "scene_composite_layout";

  m_compositeLayout.reset(hina_create_bind_group_layout(&layout_desc));
  if (!hina_bind_group_layout_is_valid(m_compositeLayout.get())) {
    LOG_ERROR("[SceneRenderFeature] Failed to create composite bind group layout");
    hslc_hsl_module_free(module);
    return false;
  }

  // Fullscreen quad vertex layout: pos(2) + uv(2)
  hina_vertex_layout vertex_layout = {};
  vertex_layout.buffer_count = 1;
  vertex_layout.buffer_strides[0] = sizeof(float) * 4;
  vertex_layout.input_rates[0] = HINA_VERTEX_INPUT_RATE_VERTEX;
  vertex_layout.attr_count = 2;
  vertex_layout.attrs[0] = { HINA_FORMAT_R32G32_SFLOAT, 0, 0, 0 };  // position
  vertex_layout.attrs[1] = { HINA_FORMAT_R32G32_SFLOAT, sizeof(float) * 2, 1, 0 };  // uv

  // Pipeline descriptor - output to HDR SCENE_COLOR format
  hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
  pip_desc.layout = vertex_layout;
  pip_desc.cull_mode = HINA_CULL_MODE_NONE;
  pip_desc.depth.depth_test = false;
  pip_desc.depth.depth_write = false;
  pip_desc.depth_format = HINA_FORMAT_UNDEFINED;
  pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;  // R16G16B16A16_SFLOAT
  pip_desc.color_attachment_count = 1;
  pip_desc.bind_group_layouts[0] = m_compositeLayout.get();
  pip_desc.bind_group_layout_count = 1;

  m_compositePipeline.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_compositePipeline.get())) {
    LOG_ERROR("[SceneRenderFeature] Composite pipeline creation failed");
    return false;
  }

  m_compositePipelineCreated = true;
  LOG_INFO("[SceneRenderFeature] Composite pipeline created successfully");
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
  LOG_INFO("[SceneRenderFeature] Composite bind group created ({}x{})", gbufferWidth, gbufferHeight);
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
    LOG_ERROR("[SceneRenderFeature] SCENE_COLOR texture not valid");
    return;
  }

  // Get depth view for sampling
  gfx::TextureView sceneDepthView = hina_texture_get_default_view(sceneDepth);

  // Ensure composite bind group is created/updated
  if (!EnsureCompositeBindGroup(gfxRenderer, sceneDepth, sceneDepthView)) {
    return;
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

  // Bind pipeline and persistent composite bind group
  hina_cmd_bind_pipeline(cmd, m_compositePipeline.get());
  hina_cmd_bind_group(cmd, 0, m_compositeBindGroup);

  // Prepare lighting push constants
  struct {
    glm::vec4 lightDir;      // xyz = direction, w = intensity
    glm::vec4 lightColor;    // rgb = color, a = unused
    glm::vec4 ambientColor;  // rgb = ambient, a = unused
    glm::vec4 cameraPos;     // xyz = camera position, w = unused
    glm::mat4 invViewProj;   // For world position reconstruction
  } lightingParams;

  // Set up directional light (sun-like, coming from upper-right-front)
  glm::vec3 lightDirection = glm::normalize(glm::vec3(0.5f, 0.8f, 0.3f));
  lightingParams.lightDir = glm::vec4(lightDirection, 1.5f);  // intensity = 1.5
  lightingParams.lightColor = glm::vec4(1.0f, 0.98f, 0.95f, 1.0f);  // warm white
  lightingParams.ambientColor = glm::vec4(0.15f, 0.17f, 0.2f, 1.0f);  // cool ambient

  // Camera position and inverse view-projection from FrameData
  // projMatrix already has Vulkan Y-flip applied at source (UploadToPipeline)
  lightingParams.cameraPos = glm::vec4(frameData.cameraPos, 1.0f);

  glm::mat4 viewProj = frameData.projMatrix * frameData.viewMatrix;
  lightingParams.invViewProj = glm::inverse(viewProj);

  hina_cmd_push_constants(cmd, 0, sizeof(lightingParams), &lightingParams);

  // Bind fullscreen quad vertex buffer
  hina_vertex_input vertInput = {};
  vertInput.vertex_buffers[0] = m_fullscreenQuadVB;
  vertInput.vertex_offsets[0] = 0;
  hina_cmd_apply_vertex_input(cmd, &vertInput);

  // Draw fullscreen quad (6 vertices)
  hina_cmd_draw(cmd, 6, 1, 0, 0);
}

