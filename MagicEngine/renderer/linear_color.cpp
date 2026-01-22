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

  // NOTE: Tone mapping pipeline is now handled by RenderGraph::ExecuteResolveViewOutput()
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

void LinearColorSystem::RegisterToneMappingPass(internal::RenderPassBuilder& /*builder*/)
{
  // NOTE: Tone mapping is now integrated into RenderGraph::ExecuteResolveViewOutput()
  // which reads SCENE_COLOR (HDR) and writes to VIEW_OUTPUT (LDR).
  // The old standalone tone mapping pass had a read/write conflict on SCENE_COLOR
  // because it tried to sample and render to the same texture simultaneously.
  // This stub is kept for API compatibility.
}

hina_format LinearColorSystem::GetSceneColorFormat() const
{
  if (m_requiresLinearWorkflow)
  {
    return LinearColor::HDR_SCENE_FORMAT;
  }
  return m_context ? static_cast<hina_format>(m_context->getSwapchainFormat()) : HINA_FORMAT_R8G8B8A8_SRGB;
}

// NOTE: Tone mapping shader and CreateToneMappingPipeline() were removed.
// Tone mapping is now handled by RenderGraph::ExecuteResolveViewOutput()
// using the Assets/shaders/resolve_output.hina_sl shader file.
