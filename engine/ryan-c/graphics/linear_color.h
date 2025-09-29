// graphics/linear_color.h
#pragma once

#include "interface.h"

class RenderGraph;

namespace internal
{
  class RenderPassBuilder;
}

namespace LinearColor
{
  // Standard linear color formats
  constexpr vk::Format HDR_SCENE_FORMAT = vk::Format::RGBA_F16;
  constexpr vk::Format LDR_FORMAT = vk::Format::RGBA_UN8;

  // Texture usage recommendations
  constexpr int HDR_SCENE_USAGE =
    vk::TextureUsageBits_Attachment |
    vk::TextureUsageBits_Sampled |
    vk::TextureUsageBits_Storage;

  constexpr int LDR_USAGE =
    vk::TextureUsageBits_Attachment |
    vk::TextureUsageBits_Sampled;

  /**
   * Helper to determine if a texture format should use sRGB for automatic conversion
   * Returns true for albedo/diffuse textures, false for data textures
   */
  bool ShouldUseSRGBFormat(const char* textureName);

  /**
   * Get the appropriate format for a given texture type
   * @param isAlbedo - true for color/albedo textures, false for data textures
   * @param preferSRGB - whether to prefer sRGB formats when appropriate
   */
  vk::Format GetTextureFormat(bool isAlbedo, bool preferSRGB = true);
}

/**
 * Linear color pipeline manager
 * Handles HDR → LDR conversion and gamma correction automatically based on swapchain color space
 * Access should go through Renderer interface, not directly.
 */

struct ToneMappingSettings
{
  enum Mode
  {
    None = 0,
    Reinhard = 1,
    Uchimura = 2,
    ACES = 3,
    KhronosPBR = 4,
    Uncharted2 = 5,
    Unreal = 6
  };

  Mode mode = KhronosPBR;
  float exposure = 0.95f;
  float maxWhite = 1.0f; // Reinhard

  // Uchimura parameters
  float P = 1.0f;  // max display brightness
  float a = 1.05f; // contrast
  float m = 0.1f;  // linear section start
  float l = 0.8f;  // linear section length
  float c = 3.0f;  // black tightness
  float b = 0.0f;  // pedestal
};

class LinearColorSystem
{
  public:

  explicit LinearColorSystem(vk::IContext& context);
  ~LinearColorSystem() = default;

  // Register linear color resources - automatically detects if linear workflow is needed
  void RegisterLinearColorResources(RenderGraph& renderGraph);

  // Register tone mapping pass - only if linear workflow is active
  void RegisterToneMappingPass(internal::RenderPassBuilder& builder);

  // Query if linear workflow is active
  bool IsLinearWorkflowActive() const {
    return m_requiresLinearWorkflow;
  }

  // Get the scene color format that features should target
  vk::Format GetSceneColorFormat() const;

  // Settings access - should be accessed through Renderer interface
  const ToneMappingSettings& GetToneMappingSettings() const {
    return m_toneMappingSettings;
  }
  void UpdateToneMappingSettings(const ToneMappingSettings& newSettings) {
    m_toneMappingSettings = newSettings;
  }

  private:
  vk::IContext& m_context;
  bool m_requiresLinearWorkflow = false; // Determined by swapchain color space
  ToneMappingSettings m_toneMappingSettings;

  // Tone mapping pipeline
  vk::Holder<vk::ShaderModuleHandle> m_vertToneMap;
  vk::Holder<vk::ShaderModuleHandle> m_fragToneMap;
  vk::Holder<vk::RenderPipelineHandle> m_pipelineToneMap;

  void CreateToneMappingPipeline();
};