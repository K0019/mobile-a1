// OIT.h - Order Independent Transparency (WBOIT stub)
//
// TODO: Implement Weighted Blended OIT for mobile-friendly transparency.
// Current implementation is a no-op placeholder.
//
// WBOIT requires only 2 render targets (fits mobile tile budget):
// - RT0 (RGBA16F): Accumulation - premultiplied weighted color + weight sum
// - RT1 (R8): Reveal - product of (1 - alpha) values
//
// See: HINA_VK_MIGRATION_PLAN.md section "Transparency - Weighted Blended OIT"

#pragma once

namespace internal
{
  class RenderPassBuilder;
}

class HinaContext;

// WBOIT resource names for RenderGraph
namespace WBOITResources
{
  constexpr const char* ACCUMULATION = "WBOIT_Accumulation";  // RGBA16F
  constexpr const char* REVEAL = "WBOIT_Reveal";              // R8
}

// Stub for future WBOIT implementation
class OITSystem
{
public:
  struct Settings
  {
    bool enabled = false;  // Disabled until implemented
  };

  OITSystem() = default;
  ~OITSystem() = default;

  void Initialize(HinaContext* /*context*/) { /* TODO: Create WBOIT pipelines */ }
  void SetupPasses(internal::RenderPassBuilder& /*passBuilder*/) { /* TODO: Register WBOIT passes */ }

  const Settings& GetSettings() const { return m_settings; }
  void UpdateSettings(const Settings& newSettings) { m_settings = newSettings; }
  bool IsInitialized() const { return false; }

private:
  Settings m_settings;
};
