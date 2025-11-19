// graphics/oit_system.h
#pragma once
#include "interface.h"

namespace internal
{
  class RenderPassBuilder;
}

namespace OITResources
{
  // OIT resource names - used by transparent features
  constexpr const char* FRAGMENT_BUFFER = "OIT_FragmentBuffer";
  constexpr const char* HEAD_TEXTURE = "OIT_HeadTexture";
  constexpr const char* ATOMIC_COUNTER = "OIT_AtomicCounter";
  constexpr const char* OIT_BUFFER = "OIT_Buffer"; // New: packed OIT metadata buffer
}

namespace OIT
{
  // Fragment data structure matching the demo
  struct TransparentFragment
  {
    uint64_t color; // f16vec4 packed
    float depth;
    uint32_t next;
  };

  // Packed OIT buffer structure
  struct OITBuffer
  {
    uint64_t bufferAtomicCounter;
    uint64_t bufferTransparencyLists;
    uint32_t texHeadsOIT;
    uint32_t maxOITFragments;
  };
} // namespace OIT
class OITSystem
{
public:
  struct Settings
  {
    uint32_t fragmentsPerPixel = 8;
    bool enableHeatmap = false;
    float opacityBoost = 0.0f;
  };

  explicit OITSystem(vk::IContext& context);

  ~OITSystem() = default;

  void SetupPasses(internal::RenderPassBuilder& passBuilder);

  const Settings& GetSettings() const { return m_settings; }

  void UpdateSettings(const Settings& newSettings) { m_settings = newSettings; }

private:
  vk::IContext& m_context;
  Settings m_settings;
  vk::Holder<vk::ShaderModuleHandle> m_vertOIT;
  vk::Holder<vk::ShaderModuleHandle> m_fragOIT;
  vk::Holder<vk::RenderPipelineHandle> m_pipelineOIT;

  void CreateOITPipeline();
};
