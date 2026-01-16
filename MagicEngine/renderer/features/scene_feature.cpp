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
#include "Graphics/LightComponent.h"
#include "ECS/ECS.h"
#include "logging/log.h"
#include <cstring>
#include <cmath>

// =============================================================================
// G-Buffer Shader (embedded hina_sl)
// =============================================================================

// G-buffer shader - Uses UBOs instead of push constants for mobile optimization
// Set 0: Frame UBO, Set 1: Material (constants UBO + textures), Set 2: Dynamic transform UBO
static const char* kGBufferShader = R"(#hina
group Frame = 0;
group Material = 1;
group Dynamic = 2;

bindings(Frame, start=0) {
  uniform(std140) FrameUBO {
    mat4 viewProj;
  } frame;
}

// Material bind group: binding 0 = packed constants, bindings 1-5 = textures
bindings(Material, start=0) {
  uniform(std140) MaterialConstants {
    uvec4 packed;  // [0]: baseColor.rg (half2), [1]: baseColor.b,roughness (half2), [2]: metallic,emissive (half2), [3]: rim,ao (half2)
  } material;
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_metallicRoughness;
  texture sampler2D u_emissive;
  texture sampler2D u_occlusion;
}

// Dynamic bind group: per-draw transform with dynamic offset
bindings(Dynamic, start=0) {
  uniform(std140) TransformUBO {
    mat4 model;
  } transform;
}

// Split vertex streams (24 bytes total):
// Buffer 0: Position stream (12 bytes)
// Buffer 1: Attribute stream (12 bytes packed) - main branch CompressedVertex encoding
struct VertexIn {
  vec3 a_position;    // location 0, buffer 0: position
  uint a_normalX;     // location 1, buffer 1: lower 16 bits = SNORM16 |nx| * nz_sign
  uint a_packed;      // location 2, buffer 1: RGB10A2 [ny, tx, ty, (nx_sign<<1)|handedness]
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
    vec4 worldPos = transform.model * vec4(in.a_position, 1.0);
    out.worldPos = worldPos.xyz;

    // Decode normal using main branch's CompressedVertex encoding
    // a_normalX lower 16 bits = SNORM16: |nx| with nz_sign in sign bit
    int normal_x_snorm = int(in.a_normalX & 0xFFFFu);
    if (normal_x_snorm > 32767) normal_x_snorm -= 65536;  // Sign extend
    float normal_x_float = float(normal_x_snorm) / 32767.0;

    float nx_magnitude = abs(normal_x_float);
    float nz_sign = normal_x_float < 0.0 ? -1.0 : 1.0;

    // a_packed = RGB10A2: [ny(10), tx(10), ty(10), alpha(2)]
    // alpha = (nx_sign_bit << 1) | handedness_bit
    float ny = float(in.a_packed & 0x3FFu) / 1023.0 * 2.0 - 1.0;
    float tx = float((in.a_packed >> 10u) & 0x3FFu) / 1023.0 * 2.0 - 1.0;
    float ty = float((in.a_packed >> 20u) & 0x3FFu) / 1023.0 * 2.0 - 1.0;
    uint alpha = (in.a_packed >> 30u) & 0x3u;

    float nx_sign = ((alpha >> 1u) & 1u) != 0u ? -1.0 : 1.0;
    float handedness = (alpha & 1u) != 0u ? -1.0 : 1.0;

    float nx = nx_magnitude * nx_sign;
    float nz = sqrt(max(0.001, 1.0 - nx * nx - ny * ny)) * nz_sign;
    vec3 localNormal = normalize(vec3(nx, ny, nz));

    // Tangent: tz is always positive (flipped during encoding if needed)
    float tz = sqrt(max(0.001, 1.0 - tx * tx - ty * ty));
    vec3 localTangent = normalize(vec3(tx, ty, tz));

    // Proper normal matrix handles non-uniform scale
    mat3 normalMatrix = transpose(inverse(mat3(transform.model)));
    out.normal = normalize(normalMatrix * localNormal);
    out.tangent = normalize(normalMatrix * localTangent);

    out.bitangentSign = handedness;

    // Unpack UV from RG16_UNORM
    out.uv = unpackUnorm2x16(in.a_uv);

    gl_Position = frame.viewProj * worldPos;
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;

    // Unpack base color from material constants (half-float packed in uvec4)
    vec2 rg = unpackHalf2x16(material.packed.x);
    vec2 bRoug = unpackHalf2x16(material.packed.y);
    vec4 baseColor = vec4(rg.x, rg.y, bRoug.x, 1.0);

    // Sample albedo texture and multiply by base color
    vec4 albedo = texture(u_albedo, in.uv) * baseColor;

    // Build TBN with original (non-negated) normal for correct normal mapping
    vec3 N = normalize(in.normal);
    vec3 T = normalize(in.tangent);
    vec3 B = normalize(cross(N, T) * in.bitangentSign);

    // Check if normal map is non-flat (default flat normal is (0.5, 0.5, 1.0))
    vec3 normalSample = texture(u_normal, in.uv).rgb;
    bool hasNormalMap = abs(normalSample.z - 1.0) > 0.01 || abs(normalSample.x - 0.5) > 0.01 || abs(normalSample.y - 0.5) > 0.01;

    if (hasNormalMap) {
        mat3 TBN = mat3(T, B, N);
        vec3 normalMap = normalSample * 2.0 - 1.0;
        N = normalize(TBN * normalMap);
    }

    // Negate final normal after TBN transformation (matches main branch's calculateLighting)
    N = -N;

    // Sample metallic/roughness (B=metallic, G=roughness in glTF convention)
    vec2 mr = texture(u_metallicRoughness, in.uv).bg;
    float metallic = mr.x;
    float roughness = mr.y;

    // Sample occlusion (R channel)
    float ao = texture(u_occlusion, in.uv).r;

    // Output albedo from material texture
    out.albedo = albedo;

    // Encode world-space normal using octahedral encoding (fits in RG16F)
    // Project to octahedron
    vec3 n = N / (abs(N.x) + abs(N.y) + abs(N.z));
    // Fold lower hemisphere
    if (n.z < 0.0) {
        float nx = n.x;
        float ny = n.y;
        n.x = (1.0 - abs(ny)) * (nx >= 0.0 ? 1.0 : -1.0);
        n.y = (1.0 - abs(nx)) * (ny >= 0.0 ? 1.0 : -1.0);
    }
    // Store in RG channels (SFLOAT, so keep in [-1,1] range)
    out.normal = vec4(n.xy, 0.0, 1.0);

    // Material data: roughness, metallic, ao, emissive intensity
    out.materialData = vec4(roughness, metallic, ao, 0.0);

    return out;
}
#hina_end
)";

// =============================================================================
// Composite Shader (embedded hina_sl) - Deferred lighting pass
// =============================================================================

// Composite shader with multi-light support via UBO
// Set 0: G-buffer textures
// Set 1: Light UBO with all scene lights
static const char* kCompositeShader = R"(#hina
group GBuffer = 0;
group Lights = 1;

bindings(GBuffer, start=0) {
  texture sampler2D u_albedo;
  texture sampler2D u_normal;
  texture sampler2D u_materialData;
  texture sampler2D u_depth;
}

// Light UBO - each light is 4 vec4s:
//   [0]: type, position.xyz (type: 0=dir, 1=point, 2=spot)
//   [1]: direction.xyz, intensity
//   [2]: color.rgb, radius
//   [3]: attenuation.xyz, unused
// Max 32 lights = 512 bytes for lights + 80 bytes header = 592 bytes
bindings(Lights, start=0) {
  uniform(std140) LightUBO {
    vec4 ambientColor;   // rgb = ambient, a = unused
    vec4 cameraPos;      // xyz = camera position, w = unused
    mat4 invViewProj;    // For world position reconstruction
    ivec4 lightCounts;   // x = numLights, yzw = unused
    vec4 lights[128];    // 32 lights * 4 vec4s each
  } ubo;
}

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

    vec2 gbufferUV = in.uv;

    // Sample G-buffer
    vec4 albedoSample = texture(u_albedo, gbufferUV);
    vec3 normalSample = texture(u_normal, gbufferUV).xyz;
    vec4 matData = texture(u_materialData, gbufferUV);
    float depth = texture(u_depth, gbufferUV).r;

    // Early out for sky/background
    if (depth >= 1.0) {
        out.color = vec4(0.1, 0.1, 0.15, 1.0);
        return out;
    }

    // Decode normal from octahedral encoding (RG16F stores [-1,1] directly)
    vec2 f = normalSample.xy;
    vec3 normal = vec3(f.x, f.y, 1.0 - abs(f.x) - abs(f.y));
    // Unfold lower hemisphere
    float t = max(-normal.z, 0.0);
    normal.x += (normal.x >= 0.0) ? -t : t;
    normal.y += (normal.y >= 0.0) ? -t : t;
    normal = normalize(normal);  // TEST: no negation

    // Material properties
    float roughness = matData.r;

    // Reconstruct world position from depth
    vec2 ndc = gbufferUV * 2.0 - 1.0;
    vec4 clipPos = vec4(ndc, depth, 1.0);
    vec4 worldPos4 = ubo.invViewProj * clipPos;
    vec3 worldPos = worldPos4.xyz / worldPos4.w;

    vec3 viewDir = normalize(ubo.cameraPos.xyz - worldPos);
    vec3 baseColor = albedoSample.rgb;
    float specPower = mix(8.0, 128.0, 1.0 - roughness);

    // Start with ambient
    vec3 totalDiffuse = vec3(0.0);
    vec3 totalSpecular = vec3(0.0);

    int numLights = ubo.lightCounts.x;
    for (int i = 0; i < numLights && i < 32; ++i) {
        int base = i * 4;
        vec4 data0 = ubo.lights[base + 0];  // type, position.xyz
        vec4 data1 = ubo.lights[base + 1];  // direction.xyz, intensity
        vec4 data2 = ubo.lights[base + 2];  // color.rgb, radius

        int lightType = int(data0.x);
        vec3 lightPos = data0.yzw;
        vec3 lightDir = data1.xyz;
        float intensity = data1.w;
        vec3 lightColor = data2.rgb;
        float radius = data2.a;

        vec3 L;
        float attenuation = 1.0;

        if (lightType == 0) {
            // Directional light - negate direction (lightDir points FROM light, L should point TO light)
            L = -normalize(lightDir);
        } else {
            // Point light (type == 1) or spot light (type == 2)
            vec3 toLight = lightPos - worldPos;
            float dist = length(toLight);
            L = toLight / max(dist, 0.001);

            // Distance attenuation with smooth falloff
            if (radius > 0.0) {
                attenuation = 1.0 - smoothstep(0.0, radius, dist);
                attenuation *= attenuation;
            } else {
                // Fallback inverse square
                attenuation = 1.0 / (1.0 + dist * dist * 0.01);
            }
        }

        // Standard one-sided diffuse (Lambert) - matches main branch
        float NdotL = max(dot(normal, L), 0.0);
        totalDiffuse += lightColor * NdotL * intensity * attenuation;

        // Standard specular (Blinn-Phong)
        vec3 halfVec = normalize(L + viewDir);
        float NdotH = max(dot(normal, halfVec), 0.0);
        float spec = pow(NdotH, specPower) * (1.0 - roughness) * 0.5;
        totalSpecular += lightColor * spec * intensity * attenuation;
    }

    // Final color
    vec3 ambient = ubo.ambientColor.rgb;
    vec3 finalColor = baseColor * (ambient + totalDiffuse) + totalSpecular;

    // DEBUG: Visualize normals as color (comment out for normal rendering)
    //out.color = vec4(normal * 0.5 + 0.5, 1.0);
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

  m_drawList.clear();
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
    auto& meshStorage = gfxRenderer->getMeshStorage();
    auto& materialSystem = gfxRenderer->getMaterialSystem();

    // Get frame resources for transform ring buffer
    auto& frame = gfxRenderer->getCurrentFrameResources();

    // Use persistently mapped pointer (mapped once during creation)
    if (!frame.transformRingMapped) {
      LOG_ERROR("[SceneRenderFeature] Transform ring buffer not mapped");
      hina_cmd_end_pass(cmd);
      return;
    }

    for (const auto& draw : m_drawList) {
      const gfx::GpuMesh* gpuMesh = meshStorage.get(draw.gfxMesh);
      if (!gpuMesh || !gpuMesh->valid) continue;

      // Check transform ring buffer overflow
      if (frame.transformRingOffset >= TRANSFORM_UBO_SIZE) {
        LOG_WARNING("[SceneRenderFeature] Transform ring buffer overflow, skipping remaining draws");
        break;
      }

      // Write transform to ring buffer at current offset
      glm::mat4* transformDst = reinterpret_cast<glm::mat4*>(
          static_cast<uint8_t*>(frame.transformRingMapped) + frame.transformRingOffset);
      *transformDst = draw.transform;

      // Bind split vertex streams with offsets
      hina_vertex_input meshInput = {};
      meshInput.vertex_buffers[0] = gpuMesh->vertexBuffer;
      meshInput.vertex_offsets[0] = gpuMesh->vertexOffset;
      meshInput.vertex_buffers[1] = gpuMesh->attributeBuffer;
      meshInput.vertex_offsets[1] = gpuMesh->attributeOffset;
      meshInput.index_buffer = gpuMesh->indexBuffer;
      meshInput.index_type = HINA_INDEX_UINT32;

      hina_cmd_apply_vertex_input(cmd, &meshInput);

      // Bind material at Set 1 (includes material constants UBO at binding 0)
      gfx::BindGroup materialBG;
      if (draw.hasMaterial && materialSystem.isMaterialValid(draw.gfxMaterial)) {
        materialBG = materialSystem.getMaterialBindGroup(draw.gfxMaterial);
      } else {
        materialBG = materialSystem.getMaterialBindGroup(materialSystem.getDefaultMaterial());
      }
      hina_cmd_bind_group(cmd, 1, materialBG);

      // Bind Set 2 (dynamic transform UBO) with dynamic offset
      uint32_t dynamicOffsets[1] = { frame.transformRingOffset };
      hina_cmd_bind_group_with_offsets(cmd, 2, frame.dynamicBindGroup, dynamicOffsets, 1);

      // Advance transform ring offset (256-byte aligned for mobile compatibility)
      frame.transformRingOffset += TRANSFORM_ALIGNMENT;

      // Draw with index offset
      hina_cmd_draw_indexed(cmd, gpuMesh->indexCount, 1, gpuMesh->indexOffset / sizeof(uint32_t), 0, 0);
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

  // Compile composite shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kCompositeShader, "scene_composite", &error);
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
  LOG_INFO("[SceneRenderFeature] Composite pipeline created successfully (with light UBO)");
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

