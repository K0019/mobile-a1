// graphics/oit_system.h
#pragma once
#include "gfx_interface.h"

class HinaContext;

namespace internal
{
  class RenderPassBuilder;
}

// TODO: Implement WBOIT (Weighted Blended OIT) per migration plan
// Current per-pixel linked list implementation requires:
// - Buffer device addresses (Vulkan 1.2+)
// - Bindless texture arrays
// - 64-bit atomics
// None of these are available on mobile/hina-vk.
//
// WBOIT uses only:
// - RT0 (RGBA16F): Accumulation - premultiplied weighted color + weight sum
// - RT1 (R8): Reveal - product of (1 - alpha) values
// See: HINA_VK_MIGRATION_PLAN.md section "Transparency - Weighted Blended OIT"

namespace OITResources
{
  // WBOIT resource names
  constexpr const char* ACCUMULATION = "OIT_Accumulation";  // RGBA16F
  constexpr const char* REVEAL = "OIT_Reveal";              // R8
}

class OITSystem
{
public:
  struct Settings
  {
    bool enabled = false;  // Disabled until WBOIT is implemented
    float opacityBoost = 0.0f;
  };

  OITSystem() = default;
  ~OITSystem() = default;

  // TODO: Implement with HinaContext* instead of vk::IContext&
  void Initialize(HinaContext* context);

  void SetupPasses(internal::RenderPassBuilder& passBuilder);

  const Settings& GetSettings() const { return m_settings; }
  void UpdateSettings(const Settings& newSettings) { m_settings = newSettings; }

  bool IsInitialized() const { return m_initialized; }

private:
  bool m_initialized = false;
  Settings m_settings;

  // TODO: WBOIT pipeline resources
  // gfx::Holder<gfx::Shader> m_wboitAccumVS;
  // gfx::Holder<gfx::Shader> m_wboitAccumFS;
  // gfx::Holder<gfx::Shader> m_wboitResolveVS;
  // gfx::Holder<gfx::Shader> m_wboitResolveFS;
  // gfx::Holder<gfx::Pipeline> m_wboitResolvePipeline;
};
