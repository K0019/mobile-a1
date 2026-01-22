#include "linear_color.h"
#include "render_graph.h"
#include "hina_context.h"
#include <hina_vk.h>
#include <cstring>
#include <iostream>

namespace LinearColor
{
  bool ShouldUseSRGBFormat(const char* textureName)
  {
    // Check for common albedo/diffuse texture names
    if (strstr(textureName, "albedo") || strstr(textureName, "diffuse") ||
        strstr(textureName, "baseColor") || strstr(textureName, "color"))
    {
      return true;
    }
    // Data textures should not use sRGB
    if (strstr(textureName, "normal") || strstr(textureName, "roughness") ||
        strstr(textureName, "metallic") || strstr(textureName, "occlusion") ||
        strstr(textureName, "height") || strstr(textureName, "displacement"))
    {
      return false;
    }
    return false;
  }

  hina_format GetTextureFormat(bool isAlbedo, bool preferSRGB)
  {
    if (isAlbedo && preferSRGB)
    {
      return HINA_FORMAT_R8G8B8A8_SRGB;
    }
    return HINA_FORMAT_R8G8B8A8_UNORM;
  }
} // namespace LinearColor

void LinearColorSystem::ensureInitialized()
{
  if (m_initialized || !m_context)
  {
    return;
  }

  if (!m_context->hasSwapchain())
  {
    m_requiresLinearWorkflow = true;
    LOG_INFO("LinearColorSystem: No swapchain, assuming linear workflow");
  }
  else
  {
    // Check color space - sRGB non-linear requires gamma correction
    auto colorSpace = m_context->getSwapchainColorSpace();
    m_requiresLinearWorkflow = (colorSpace == gfx::ColorSpace::SRGB_NONLINEAR);
    LOG_INFO("LinearColorSystem: Linear workflow = {}", m_requiresLinearWorkflow);
  }

  CreateToneMappingPipeline();
  m_initialized = true;
}

void LinearColorSystem::RegisterLinearColorResources(RenderGraph& renderGraph)
{
  ensureInitialized();

  if (m_requiresLinearWorkflow)
  {
    // Linear workflow: Use HDR intermediate format at fixed internal resolution
    gfx::TextureDesc hdrSceneColorDesc{
      .type = HINA_TEX_TYPE_2D,
      .format = LinearColor::HDR_SCENE_FORMAT,
      .width = RenderResources::INTERNAL_WIDTH,
      .height = RenderResources::INTERNAL_HEIGHT,
      .depth = 1,
      .layers = 1,
      .mip_levels = 1,
      .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled | gfx::TextureUsage::TransferSrc)
    };
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, hdrSceneColorDesc);
  }
  else
  {
    // Direct rendering: Use swapchain-compatible format
    gfx::TextureDesc directSceneColorDesc{
      .type = HINA_TEX_TYPE_2D,
      .format = static_cast<hina_format>(m_context->getSwapchainFormat()),
      .width = RenderResources::INTERNAL_WIDTH,
      .height = RenderResources::INTERNAL_HEIGHT,
      .depth = 1,
      .layers = 1,
      .mip_levels = 1,
      .usage = static_cast<hina_texture_usage_flags>(gfx::TextureUsage::RenderTarget | gfx::TextureUsage::Sampled | gfx::TextureUsage::TransferSrc)
    };
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, directSceneColorDesc);
  }
}

void LinearColorSystem::RegisterToneMappingPass(internal::RenderPassBuilder& builder)
{
  // TODO: Tone mapping is disabled for now because the pass tries to read and write
  // the same texture (SCENE_COLOR) simultaneously, which is undefined behavior.
  // This needs architectural changes to work properly - read from HDR source and
  // write to a separate LDR target.
  if (!m_requiresLinearWorkflow) return;
  if (m_toneMappingSettings.mode == ToneMappingSettings::Passthrough) return;

  PassDeclarationInfo tonemap;
  tonemap.framebufferDebugName = "ToneMapping";
  tonemap.colorAttachments[0] = {
    .textureName = RenderResources::SCENE_COLOR,
    .loadOp = gfx::LoadOp::Load,
    .storeOp = gfx::StoreOp::Store
  };

  builder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(599))
    .AddGraphicsPass("ToneMapping", tonemap, [this](internal::ExecutionContext& context)
    {
      if (!gfx::isValid(m_pipelineToneMap.get())) return;

      auto& cmd = context.GetCommandBuffer();
      gfx::Texture sceneColor = context.GetTexture(RenderResources::SCENE_COLOR);
      if (!gfx::isValid(sceneColor)) return;

      // Create transient bind group for HDR scene color
      gfx::TextureView sceneView = hina_texture_get_default_view(sceneColor);
      hina_transient_bind_group tbg = hina_alloc_transient_bind_group(m_toneMapBindGroupLayout.get());
      hina_transient_write_combined_image(&tbg, 0, sceneView, m_toneMapSampler.get());
      hina_cmd_bind_transient_group(context.GetCmd(), 0, tbg);

      cmd.bindPipeline(m_pipelineToneMap.get());

      struct ToneMappingPC
      {
        float exposure;
        int mode;
        float maxWhite;
        float P, a, m, l, c, b;
      } pc = {
        .exposure = m_toneMappingSettings.exposure,
        .mode = static_cast<int>(m_toneMappingSettings.mode),
        .maxWhite = m_toneMappingSettings.maxWhite,
        .P = m_toneMappingSettings.P,
        .a = m_toneMappingSettings.a,
        .m = m_toneMappingSettings.m,
        .l = m_toneMappingSettings.l,
        .c = m_toneMappingSettings.c,
        .b = m_toneMappingSettings.b,
      };
      cmd.pushConstants(pc);
      cmd.draw(3);
    });
}

hina_format LinearColorSystem::GetSceneColorFormat() const
{
  if (m_requiresLinearWorkflow)
  {
    return LinearColor::HDR_SCENE_FORMAT;
  }
  return m_context ? static_cast<hina_format>(m_context->getSwapchainFormat()) : HINA_FORMAT_R8G8B8A8_SRGB;
}

// Tone mapping shader in HSL format
static const char* kToneMappingShader = R"(#hina
group ToneMap = 0;

bindings(ToneMap, start=0) {
  texture sampler2D hdrSceneColor;
}

push_constant ToneMappingData {
  float exposure;
  int mode;
  float maxWhite;
  float P;
  float a;
  float m;
  float l;
  float c;
  float b;
} pc;

struct Varyings {
  vec2 uv;
};

struct FragOut {
  vec4 color;
};
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain() {
  Varyings out;
  out.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
  gl_Position = vec4(out.uv * vec2(2, -2) + vec2(-1, 1), 0.0, 1.0);
  return out;
}
#hina_end

#hina_stage fragment entry FSMain
vec3 ACESFilm(vec3 x) {
  const float a = 2.51;
  const float b = 0.03;
  const float c = 2.43;
  const float d = 0.59;
  const float e = 0.14;
  return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Reinhard(vec3 color) {
  return color / (1.0 + color / pc.maxWhite);
}

vec3 Uncharted2(vec3 x) {
  const float A = 0.15;
  const float B = 0.50;
  const float C = 0.10;
  const float D = 0.20;
  const float E = 0.02;
  const float F = 0.30;
  return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

vec3 Unreal(vec3 x) {
  return x / (x + 0.155) * 1.019;
}

vec3 KhronosPBR(vec3 color) {
  const float startCompression = 0.76;
  const float desaturation = 0.15;
  float x = min(color.r, min(color.g, color.b));
  float offset = x < 0.08 ? x - 6.25 * x * x : 0.04;
  color -= offset;
  float peak = max(color.r, max(color.g, color.b));
  if (peak < startCompression) return color;
  const float d = 1.0 - startCompression;
  float newPeak = 1.0 - d * d / (peak + d - startCompression);
  color *= newPeak / peak;
  float g = 1.0 - 1.0 / (desaturation * (peak - newPeak) + 1.0);
  return mix(color, vec3(newPeak), g);
}

vec3 Uchimura(vec3 x) {
  float P = pc.P;
  float a = pc.a;
  float m = pc.m;
  float l = pc.l;
  float c = pc.c;
  float b = pc.b;
  float l0 = ((P - m) * l) / a;
  float S1 = m + (a * l0);
  float C2 = (a * P) / (P - S1);
  float CP = -C2 / P;
  float S0 = m + l0;
  vec3 result;
  for(int i = 0; i < 3; i++) {
    float val = x[i];
    if(val < m) {
      result[i] = m * pow(val / m, c) + b;
    } else if(val < m + l0) {
      result[i] = m + a * (val - m);
    } else {
      result[i] = P - (P - S1) * exp(CP * (val - S0));
    }
  }
  return result;
}

FragOut FSMain(Varyings in) {
  FragOut out;
  vec3 hdrColor = texture(hdrSceneColor, in.uv).rgb;

  // Passthrough mode - no processing
  if (pc.mode == 7) {
    out.color = vec4(hdrColor, 1.0);
    return out;
  }

  // Apply exposure
  hdrColor *= pc.exposure;

  // Apply tone mapping
  vec3 toneMapped;
  if (pc.mode == 1) {
    toneMapped = Reinhard(hdrColor);
  } else if (pc.mode == 2) {
    toneMapped = Uchimura(hdrColor);
  } else if (pc.mode == 3) {
    toneMapped = ACESFilm(hdrColor);
  } else if (pc.mode == 4) {
    toneMapped = KhronosPBR(hdrColor);
  } else if (pc.mode == 5) {
    toneMapped = Uncharted2(hdrColor);
  } else if (pc.mode == 6) {
    toneMapped = Unreal(hdrColor);
    out.color = vec4(toneMapped, 1.0);
    return out;
  } else {
    toneMapped = clamp(hdrColor, 0.0, 1.0);
  }

  // Gamma correction (sRGB conversion)
  toneMapped = pow(toneMapped, vec3(1.0 / 2.2));
  out.color = vec4(toneMapped, 1.0);
  return out;
}
#hina_end
)";

void LinearColorSystem::CreateToneMappingPipeline()
{
  if (!m_requiresLinearWorkflow || !m_context) return;

  std::cout << "[LinearColor] Creating tone mapping pipeline..." << std::endl;

  // Compile HSL shader
  char* error = nullptr;
  hina_hsl_module* module = hslc_compile_hsl_source(kToneMappingShader, "tonemap_shader", &error);
  if (!module) {
    std::cout << "[LinearColor] Shader compilation failed: " << (error ? error : "Unknown") << std::endl;
    if (error) hslc_free_log(error);
    return;
  }
  std::cout << "[LinearColor] Shader compiled successfully" << std::endl;

  // Create sampler
  {
    gfx::SamplerDesc samplerDesc = gfx::samplerDescDefault();
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_w = HINA_ADDRESS_CLAMP_TO_EDGE;
    m_toneMapSampler.reset(gfx::createSampler(samplerDesc));
  }

  // Create pipeline using HSL module - let HSL reflection create bind group layouts
  {
    hina_hsl_pipeline_desc pip_desc = hina_hsl_pipeline_desc_default();
    pip_desc.cull_mode = HINA_CULL_MODE_NONE;
    pip_desc.depth.depth_test = false;
    pip_desc.depth.depth_write = false;
    pip_desc.depth_format = HINA_FORMAT_UNDEFINED;

    // Output to HDR scene color format (matches SCENE_COLOR resource)
    pip_desc.color_formats[0] = LinearColor::HDR_SCENE_FORMAT;
    pip_desc.color_attachment_count = 1;

    // Don't set explicit bind group layouts - HSL reflection handles this
    pip_desc.bind_group_layout_count = 0;

    m_pipelineToneMap.reset(hina_make_pipeline_from_module(module, &pip_desc, nullptr));
  }

  hslc_hsl_module_free(module);

  if (!hina_pipeline_is_valid(m_pipelineToneMap.get())) {
    std::cout << "[LinearColor] Pipeline creation failed" << std::endl;
    return;
  }

  // Get the bind group layout from the pipeline (created by HSL reflection)
  m_toneMapBindGroupLayout.reset(hina_pipeline_get_bind_group_layout(m_pipelineToneMap.get(), 0));

  std::cout << "[LinearColor] Pipeline created successfully" << std::endl;
}
