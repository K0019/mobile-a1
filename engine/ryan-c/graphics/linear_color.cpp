#include "linear_color.h"

#include "render_graph.h"
#include "shader.h"

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
    
    // Default to false for safety
    return false;
  }
  
  vk::Format GetTextureFormat(bool isAlbedo, bool preferSRGB)
  {
    if (isAlbedo && preferSRGB)
    {
      return vk::Format::RGBA_SRGB8; // Automatic sRGB->linear conversion
    }
    return vk::Format::RGBA_UN8; // Linear data textures
  }
}

LinearColorSystem::LinearColorSystem(vk::IContext& context)
  : m_context(context)
{
  // Query swapchain color space to determine if linear workflow is needed
  vk::ColorSpace colorSpace = context.getSwapchainColorSpace();

  m_requiresLinearWorkflow = (colorSpace == vk::ColorSpace::SRGB_NONLINEAR);
  
  CreateToneMappingPipeline();
}

void LinearColorSystem::RegisterLinearColorResources(RenderGraph& renderGraph)
{
  if (m_requiresLinearWorkflow)
  {
    // Linear workflow: Use HDR intermediate format
    vk::TextureDesc hdrSceneColorDesc{
      .type = vk::TextureType::Tex2D,
      .format = LinearColor::HDR_SCENE_FORMAT,
      .dimensions = ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS,
      .usage = LinearColor::HDR_SCENE_USAGE,
      .debugName = "HDR Scene Color"
    };
    
    // Register HDR scene color - this replaces the basic SCENE_COLOR
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, hdrSceneColorDesc);
  }
  else
  {
    // Direct rendering: Use swapchain-compatible format
    vk::TextureDesc directSceneColorDesc{
      .type = vk::TextureType::Tex2D,
      .format = m_context.getSwapchainFormat(),
      .dimensions = ResourceProperties::SWAPCHAIN_RELATIVE_DIMENSIONS,
      .usage = vk::TextureUsageBits_Attachment | vk::TextureUsageBits_Sampled,
      .debugName = "Direct Scene Color"
    };
    
    renderGraph.RegisterTransientResource(RenderResources::SCENE_COLOR, directSceneColorDesc);
  }
}

void LinearColorSystem::RegisterToneMappingPass(internal::RenderPassBuilder& builder)
{
  if (!m_requiresLinearWorkflow) return ;

  // Option 1: In-place tonemapping (recommended)
  builder.CreatePass()
    .UseResource(RenderResources::SCENE_COLOR, AccessType::ReadWrite)
    .SetPriority(static_cast<internal::RenderPassBuilder::PassPriority>(599)) // Just before all UI passes.
    .AddGenericPass("ToneMapping", [this](internal::ExecutionContext& context)
    {
      auto& cmd = context.GetvkCommandBuffer();
      
      cmd.cmdBindRenderPipeline(m_pipelineToneMap);
      
      struct ToneMappingPC
      {
        uint32_t hdrSceneColorIndex;
        float exposure;
        int mode;
        float maxWhite;
        float P, a, m, l, c, b;
      } pc = {
        .hdrSceneColorIndex = context.GetTexture(RenderResources::SCENE_COLOR).index(),
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

      cmd.cmdPushConstants(pc);
      cmd.cmdBindDepthState({.compareOp = vk::CompareOp::Always, .isDepthWriteEnabled = false});
      cmd.cmdDraw(3);
    });
}

vk::Format LinearColorSystem::GetSceneColorFormat() const
{
  return m_requiresLinearWorkflow ? LinearColor::HDR_SCENE_FORMAT : m_context.getSwapchainFormat();
}

void LinearColorSystem::CreateToneMappingPipeline()
{
  // Only create tone mapping pipeline if linear workflow is active
  if (!m_requiresLinearWorkflow) return;
  
  m_vertToneMap = loadShaderModule(m_context, "shaders/fullscreen.vert");
  m_fragToneMap = loadShaderModule(m_context, "shaders/tonemap.frag");
  
  m_pipelineToneMap = m_context.createRenderPipeline({
    .smVert = m_vertToneMap,
    .smFrag = m_fragToneMap,
    .color = {{ .format = m_context.getSwapchainFormat() }},
    .debugName = "Tone Mapping Pipeline"
  });
}