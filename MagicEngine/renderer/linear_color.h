// graphics/linear_color.h
#pragma once
#include "gfx_interface.h"

class RenderGraph;
class HinaContext;

namespace internal
{
  class RenderPassBuilder;
}

namespace LinearColor
{
  // Standard linear color formats
  constexpr hina_format HDR_SCENE_FORMAT = HINA_FORMAT_R16G16B16A16_SFLOAT;
  constexpr hina_format LDR_FORMAT = HINA_FORMAT_R8G8B8A8_UNORM;

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
  hina_format GetTextureFormat(bool isAlbedo, bool preferSRGB = true);
} // namespace LinearColor
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
    Unreal = 6,
    Passthrough = 7  // No processing - preserves linear color exactly
  };

  Mode mode = Passthrough;
  float exposure = 1.0f;
  float maxWhite = 1.0f; // Reinhard
  // Uchimura parameters
  float P = 1.0f; // max display brightness
  float a = 1.05f; // contrast
  float m = 0.1f; // linear section start
  float l = 0.8f; // linear section length
  float c = 3.0f; // black tightness
  float b = 0.0f; // pedestal
};

class LinearColorSystem
{
public:
  LinearColorSystem() = default;
  ~LinearColorSystem() = default;

  // Initialize with context - must be called before other methods
  void Initialize(HinaContext* context) { m_context = context; }

  // Register linear color resources - automatically detects if linear workflow is needed
  void RegisterLinearColorResources(RenderGraph& renderGraph);

  // Register tone mapping pass - only if linear workflow is active
  void RegisterToneMappingPass(internal::RenderPassBuilder& builder);

  // Query if linear workflow is active
  bool IsLinearWorkflowActive() const { return m_requiresLinearWorkflow; }

  // Get the scene color format that features should target
  hina_format GetSceneColorFormat() const;

  // Settings access
  const ToneMappingSettings& GetToneMappingSettings() const { return m_toneMappingSettings; }
  void UpdateToneMappingSettings(const ToneMappingSettings& newSettings) { m_toneMappingSettings = newSettings; }

private:
  HinaContext* m_context = nullptr;
  bool m_initialized = false;
  bool m_requiresLinearWorkflow = false;
  ToneMappingSettings m_toneMappingSettings;

  // Tone mapping pipeline resources
  gfx::Holder<gfx::Pipeline> m_pipelineToneMap;
  gfx::Holder<gfx::BindGroupLayout> m_toneMapBindGroupLayout;
  gfx::Holder<gfx::Sampler> m_toneMapSampler;

  void CreateToneMappingPipeline();
  void ensureInitialized();
};
