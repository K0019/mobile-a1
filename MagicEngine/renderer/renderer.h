#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <variant>
#include <unordered_map>
#include <atomic>
#include <functional>
#include "i_renderer.h"
#include "frame_data.h"
#include "linear_color.h"
#include "gfx_renderer.h"

// Platform-specific surface transform (Android only)
#if defined(__ANDROID__)
enum class SurfaceTransform : uint8_t
{
    Identity = 0,
    Rotate90,
    Rotate180,
    Rotate270,
    HorizontalMirror,
    HorizontalMirrorRotate90,
    HorizontalMirrorRotate180,
    HorizontalMirrorRotate270,
    Inherit,
};
#endif

class Renderer : public IRenderer
{
public:
  Renderer();

  void initialize();

  // Lifecycle - all called from main thread
  void startup();

  void shutdown();

  // Frame processing - all on main thread
  void beginFrame();

  void render(FrameData& frameData);  // Single view (legacy)
  void render(RenderFrameData& frameData);  // Multi-view (preferred)

  void endFrame();

  void onWindowResized(int width, int height);

  // Surface lifecycle management (for Android)
  void handleSurfaceCreated();

  void handleSurfaceDestroyed();

  // Feature system interface (simplified)
  void DestroyFeature(uint64_t feature_handle) override;

  void* GetFeatureParameterBlockPtr(uint64_t feature_handle) override;

  template <typename TFeature>
  TFeature* GetFeature(uint64_t feature_handle)
  {
    auto it = m_features.find(feature_handle);
    if (it == m_features.end() || !it->second.feature)
    {
      return nullptr;
    }
    return dynamic_cast<TFeature*>(it->second.feature.get());
  }

  // hina-vk backend - use GfxRenderer for mesh/material/texture management
  GfxRenderer* getGfxRenderer() { return m_gfxRenderer.get(); }
  const GfxRenderer* getGfxRenderer() const { return m_gfxRenderer.get(); }

  const ToneMappingSettings& GetToneMappingSettings() const;

  void UpdateToneMappingSettings(const ToneMappingSettings& newSettings) const;

  bool isWindowReadyForShow() const { return m_windowReadyForShow; }

#if defined(__ANDROID__)
  SurfaceTransform getSwapchainPreTransform() const;
#endif

private:
  uint64_t DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory) override;

  // Frame-in-flight management (simplified)
  static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

  struct PerFrameResources
  {
    FrameData frameData;
    // Each feature manages its own per-frame data via double-buffering
  };

  std::array<PerFrameResources, MAX_FRAMES_IN_FLIGHT> m_frameResources;
  uint32_t m_currentFrame = 0;

  struct FeatureInfo
  {
    std::unique_ptr<IRenderFeature> feature;
    uint64_t handle;

    explicit FeatureInfo(std::unique_ptr<IRenderFeature> feat, uint64_t h) : feature(std::move(feat)), handle(h)
    {
    }
  };

  std::unordered_map<uint64_t, FeatureInfo> m_features;
  std::atomic<uint64_t> m_nextFeatureHandle{1};

  // Core systems
  std::unique_ptr<GfxRenderer> m_gfxRenderer;

  int m_framesRendered = 0;
  bool m_windowReadyForShow = false;
  const int MIN_FRAMES_BEFORE_SHOW = 3;
};
