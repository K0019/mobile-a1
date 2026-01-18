#include "renderer.h"
#include "resource/resource_types.h"
#include "core/platform/platform.h"
#include "logging/log.h"
#include "logging/profiler.h"

#ifdef __ANDROID__
#include <hina_vk.h>  // For hina_recreate_surface
#endif

Renderer::Renderer() = default;

void Renderer::initialize()
{
  // Initialize frame resources
  for (auto& frame : m_frameResources)
  {
    frame.frameData = {};
  }
  LOG_INFO("Renderer created (single-threaded) - {}x{}", Core::Display().GetWidth(), Core::Display().GetHeight());
}

void Renderer::startup()
{
  PROFILER_FUNCTION();

  // Create GfxRenderer (hina-vk backend)
  m_gfxRenderer = std::make_unique<GfxRenderer>();

#ifdef __ANDROID__
  // On Android, defer GfxRenderer initialization until window is available
  // The window is not yet set when Engine::Initialize() calls startup()
  // handleSurfaceCreated() will initialize GfxRenderer when the surface is ready
  LOG_INFO("Renderer startup (Android) - deferring GfxRenderer init until surface created");
#else
  // Desktop: Window is available immediately, initialize now
  void* nativeWindow = Core::Display().GetVulkanWindowHandle();
  uint32_t width = Core::Display().GetWidth();
  uint32_t height = Core::Display().GetHeight();

  if (!m_gfxRenderer->initialize(nativeWindow, width, height))
  {
    LOG_CRITICAL("Failed to initialize GfxRenderer");
    return;
  }
  LOG_INFO("Renderer startup complete (hina-vk backend)");
#endif
}

void Renderer::shutdown()
{
  PROFILER_FUNCTION();
  LOG_INFO("Renderer shutdown starting...");

  m_features.clear();
  if (m_gfxRenderer)
  {
    m_gfxRenderer->shutdown();
    m_gfxRenderer.reset();
  }

  LOG_INFO("Renderer shutdown complete");
}

#if defined(__ANDROID__)
SurfaceTransform Renderer::getSwapchainPreTransform() const
{
  // TODO: Implement for hina-vk
  return SurfaceTransform::Identity;
}
#endif

void Renderer::beginFrame()
{
  PROFILER_FUNCTION();
  // GfxRenderer handles frame synchronization internally
  // Nothing to do here - beginFrame() on GfxRenderer is called in render()
}

void Renderer::render(FrameData& frameData)
{
  PROFILER_FUNCTION();

  if (!m_gfxRenderer || !m_gfxRenderer->isInitialized())
  {
    LOG_ERROR("Cannot render - GfxRenderer not initialized");
    return;
  }

  // Update frame data
  frameData.frameNumber = static_cast<uint64_t>(m_framesRendered);
  m_frameResources[m_currentFrame].frameData = frameData;

  // Swap buffers for features (feature system still works)
  for (auto& info : m_features)
  {
    if (info.second.feature)
    {
      info.second.feature->SwapBuffersForGT_RT();
    }
  }

  // Use GfxRenderer for rendering
  if (m_gfxRenderer->beginFrame())
  {
    m_gfxRenderer->render(m_frameResources[m_currentFrame].frameData);
    m_gfxRenderer->endFrame();
    m_framesRendered++;

    if (!m_windowReadyForShow && m_framesRendered >= MIN_FRAMES_BEFORE_SHOW)
    {
      m_windowReadyForShow = true;
    }
  }
}

void Renderer::render(RenderFrameData& frameData)
{
  PROFILER_FUNCTION();

  if (!m_gfxRenderer || !m_gfxRenderer->isInitialized())
  {
    LOG_ERROR("Cannot render - GfxRenderer not initialized");
    return;
  }

  // Update frame numbers for all views
  for (auto& view : frameData.views)
  {
    view.frameNumber = static_cast<uint64_t>(m_framesRendered);
  }

  // Swap buffers for features (feature system still works)
  for (auto& info : m_features)
  {
    if (info.second.feature)
    {
      info.second.feature->SwapBuffersForGT_RT();
    }
  }

  // Use GfxRenderer for multi-view rendering
  if (m_gfxRenderer->beginFrame())
  {
    m_gfxRenderer->render(frameData);
    m_gfxRenderer->endFrame();
    m_framesRendered++;

    if (!m_windowReadyForShow && m_framesRendered >= MIN_FRAMES_BEFORE_SHOW)
    {
      m_windowReadyForShow = true;
    }
  }
}

void Renderer::endFrame()
{
  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::onWindowResized(int width, int height)
{
  PROFILER_FUNCTION();
  if (width <= 0 || height <= 0)
  {
    return;
  }

  if (m_gfxRenderer)
  {
    m_gfxRenderer->onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    LOG_INFO("Window resized to {}x{}", width, height);
  }
}

void Renderer::handleSurfaceCreated()
{
#ifdef __ANDROID__
  void* nativeWindow = Core::Display().GetVulkanWindowHandle();
  uint32_t width = Core::Display().GetWidth();
  uint32_t height = Core::Display().GetHeight();

  if (!m_gfxRenderer)
  {
    LOG_ERROR("handleSurfaceCreated: GfxRenderer not created!");
    return;
  }

  if (!m_gfxRenderer->isInitialized())
  {
    // First time initialization on Android - this is where we actually initialize hina-vk
    LOG_INFO("handleSurfaceCreated: Initializing GfxRenderer with window={}, size={}x{}",
             nativeWindow, width, height);
    if (!m_gfxRenderer->initialize(nativeWindow, width, height))
    {
      LOG_ERROR("Failed to initialize GfxRenderer on surface created");
      return;
    }
    LOG_INFO("GfxRenderer initialized successfully on Android");
  }
  else
  {
    // Recreate surface after lifecycle event (app resumed from background)
    LOG_INFO("handleSurfaceCreated: Recreating surface");
    if (!hina_recreate_surface(nativeWindow, nullptr))
    {
      LOG_ERROR("Failed to recreate surface");
      return;
    }
    m_gfxRenderer->onResize(width, height);
    LOG_INFO("Surface recreated: {}x{}", width, height);
  }
#else
  // Desktop: Surface is stable, GfxRenderer was initialized in startup()
  if (m_gfxRenderer && m_gfxRenderer->isInitialized())
  {
    LOG_INFO("handleSurfaceCreated: Already initialized on desktop, skipping");
  }
#endif
}

void Renderer::handleSurfaceDestroyed()
{
  if (m_gfxRenderer)
  {
    m_gfxRenderer->shutdown();
    LOG_INFO("Surface destroyed - GfxRenderer shutdown");
  }
}

uint64_t Renderer::DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory)
{
  PROFILER_FUNCTION();
  uint64_t handle = m_nextFeatureHandle.fetch_add(1, std::memory_order_relaxed);
  auto feature = factory();
  if (!feature)
  {
    LOG_ERROR("Feature factory returned null");
    return 0; // Invalid handle
  }
  LOG_INFO("Creating feature '{}' with handle {}", feature->GetName(), handle);

  // Get raw pointer before moving into storage
  IRenderFeature* rawFeature = feature.get();

  // Register with GfxRenderer's RenderGraph - this will call SetupPasses during compilation
  if (m_gfxRenderer)
  {
    m_gfxRenderer->registerFeature(rawFeature);
  }

  // Get the feature mask after registration
  FeatureMask mask = m_gfxRenderer ? m_gfxRenderer->getFeatureMask(rawFeature) : 0;

  m_features.emplace(std::piecewise_construct, std::forward_as_tuple(handle),
                     std::forward_as_tuple(std::move(feature), handle, mask));

  return handle;
}

void Renderer::DestroyFeature(uint64_t feature_handle)
{
  PROFILER_FUNCTION();
  auto it = m_features.find(feature_handle);
  if (it == m_features.end())
  {
    LOG_WARNING("Attempted to destroy non-existent feature handle {}", feature_handle);
    return;
  }
  LOG_INFO("Destroying feature '{}' with handle {}", it->second.feature ? it->second.feature->GetName() : "unknown",
           feature_handle);

  // Unregister from GfxRenderer's RenderGraph before destroying
  if (m_gfxRenderer && it->second.feature)
  {
    m_gfxRenderer->unregisterFeature(it->second.feature.get());
  }

  m_features.erase(it);
}

void* Renderer::GetFeatureParameterBlockPtr(uint64_t feature_handle)
{
  auto it = m_features.find(feature_handle);
  if (it == m_features.end())
  {
    LOG_WARNING("Requested parameter block for non-existent feature handle {}", feature_handle);
    return nullptr;
  }
  if (!it->second.feature)
  {
    LOG_WARNING("Feature handle {} has null feature pointer", feature_handle);
    return nullptr;
  }
  return it->second.feature->GetGTParameterBlock_RT();
}

FeatureMask Renderer::GetFeatureMask(uint64_t feature_handle) const
{
  auto it = m_features.find(feature_handle);
  if (it == m_features.end()) return 0;
  // Return cached mask, or query from GfxRenderer if not cached
  if (it->second.mask != 0) return it->second.mask;
  if (it->second.feature && m_gfxRenderer)
  {
    return m_gfxRenderer->getFeatureMask(it->second.feature.get());
  }
  return 0;
}

const ToneMappingSettings& Renderer::GetToneMappingSettings() const
{
  // TODO: Implement tone mapping settings for GfxRenderer
  static ToneMappingSettings defaultSettings{};
  return defaultSettings;
}

void Renderer::UpdateToneMappingSettings([[maybe_unused]] const ToneMappingSettings& newSettings) const
{
  // TODO: Implement tone mapping settings for GfxRenderer
}
