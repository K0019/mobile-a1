#include "renderer.h"
#include "resource/resource_types.h"
#include "core/platform/platform.h"
#include "logging/log.h"
#include "logging/profiler.h"

Renderer::Renderer() = default;

void Renderer::initialize()
{
    // Initialize frame submit handles to empty (invalid) state
  m_frameSubmitHandles.fill(vk::SubmitHandle{});

  // Initialize frame resources
  for (auto& frame : m_frameResources)
  {
    frame.frameData = {};
  }


  LOG_INFO("Renderer created (single-threaded) - {}x{}", Core::Display().GetWidth(),  Core::Display().GetHeight());
}

void Renderer::startup()
{
  PROFILER_FUNCTION();

  // Create headless Vulkan context (no surface/swapchain)
  m_vkContext = vk::createContext({
    .vulkanVersion = vk::VulkanVersion_1_3,
    .terminateOnValidationError = false,
    .enableValidation = false,
    .swapchainRequestedColorSpace = vk::ColorSpace::SRGB_LINEAR,
    .disableVSync = true
  });

  if (!m_vkContext)
  {
    LOG_CRITICAL("Failed to create Vulkan context");
    return;
  }

  // Initialize GPU buffers
  m_gpuBuffers = std::make_unique<GPUBuffers>(
    *m_vkContext,
    ResourceLimits::VERTEX_BUFFER_SIZE,
    ResourceLimits::INDEX_BUFFER_SIZE,
    ResourceLimits::MATERIAL_BUFFER_SIZE
  );

  // Create RenderGraph (compilation deferred)
  m_renderGraph = std::make_unique<RenderGraph>(*m_vkContext, *m_gpuBuffers);

  LOG_INFO("Renderer startup complete (headless)");
}

void Renderer::shutdown()
{
  PROFILER_FUNCTION();
  LOG_INFO("Renderer shutdown starting...");
  for (const auto& handle : m_frameSubmitHandles)
  {
    if (!handle.empty())
    {
      m_vkContext->wait(handle);
    }
  }
  for (auto& info : m_features)
  {
    if (info.second.feature)
    {
      m_renderGraph->RemoveFeature(info.second.feature.get());
    }
  }
  m_features.clear();
  m_gpuBuffers.reset();
  m_renderGraph.reset();
  m_vkContext.reset();

  LOG_INFO("Renderer shutdown complete");
}

void Renderer::beginFrame()
{
  PROFILER_FUNCTION();
  const vk::SubmitHandle& previousSubmit = m_frameSubmitHandles[m_currentFrame];
  if (!previousSubmit.empty())
  {
    m_vkContext->wait(previousSubmit);
  }
}

void Renderer::render(FrameData& frameData)
{
  PROFILER_FUNCTION();

  // Validation
  if (!m_vkContext)
  {
    LOG_ERROR("Cannot render - Vulkan context is null");
    return;
  }

  // Silent early exit if no surface (normal on Android)
  if (!m_vkContext->hasSurface() || !m_vkContext->hasSwapchain())
  {
    return;
  }

  // Update frame data
  frameData.frameNumber = static_cast<uint64_t>(m_framesRendered);
  m_frameResources[m_currentFrame].frameData = frameData;

  // Swap buffers for features
  for (auto& info : m_features)
  {
    if (info.second.feature)
    {
      info.second.feature->SwapBuffersForGT_RT();
    }
  }

  // Get swapchain texture
  const vk::TextureHandle currentTexture = m_vkContext->getCurrentSwapchainTexture();
  if (!currentTexture.valid())
  {
    return; // Swapchain not ready
  }

  // Execute render graph
  vk::ICommandBuffer& cmdBuffer = m_vkContext->acquireCommandBuffer();
  m_renderGraph->Execute(cmdBuffer, m_frameResources[m_currentFrame].frameData);

  m_frameSubmitHandles[m_currentFrame] = m_vkContext->submit(cmdBuffer, currentTexture);
  m_framesRendered++;

  if (!m_windowReadyForShow && m_framesRendered >= MIN_FRAMES_BEFORE_SHOW)
  {
    m_windowReadyForShow = true;
  }
}

void Renderer::endFrame()
{
  m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Renderer::onWindowResized(int width, int height)
{
  PROFILER_FUNCTION();

  if (!m_vkContext || width <= 0 || height <= 0)
  {
    return;
  }

  // Only recreate if we have a surface
  if (m_vkContext->hasSurface())
  {
    m_vkContext->recreateSwapchain(width, height);
    LOG_INFO("Window resized to {}x{}", width, height);
  }
}

void Renderer::handleSurfaceCreated()
{
  if (!m_vkContext)
  {
    LOG_ERROR("Vulkan context not initialized");
    return;
  }

  // Create surface
  void* nativeWindow = Core::Display().GetVulkanWindowHandle();
  void* nativeDisplay = Core::Display().GetVulkanDisplayHandle(); // May be null
  m_vkContext->createSurface(nativeWindow, nativeDisplay);

  // Create swapchain
  uint32_t width = Core::Display().GetWidth();
  uint32_t height = Core::Display().GetHeight();

  m_vkContext->recreateSwapchain(width, height);
  // Trigger render graph compilation
  if (m_renderGraph)
  {
    m_renderGraph->Invalidate();
    LOG_INFO("Surface created - render graph will compile on next execute");
  }

  LOG_INFO("Surface and swapchain created: {}x{}", width, height);
}

void Renderer::handleSurfaceDestroyed()
{
  if (!m_vkContext)
  {
    return;
  }

  // Invalidate before destroying
  if (m_renderGraph)
  {
    m_renderGraph->Invalidate();
    LOG_INFO("Render graph invalidated");
  }

  m_vkContext->destroySurface();

  LOG_INFO("Surface and swapchain destroyed");
}

void Renderer::AddTransientResourceObserver(internal::ITransientResourceObserver* observer) const
{
  if (m_renderGraph && observer)
  {
    m_renderGraph->AddTransientObserver(observer);
  }
}

void Renderer::RemoveTransientResourceObserver(internal::ITransientResourceObserver* observer) const
{
  if (m_renderGraph && observer)
  {
    m_renderGraph->RemoveTransientObserver(observer);
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
  m_renderGraph->AddFeature(feature.get());
  m_features.emplace(std::piecewise_construct, std::forward_as_tuple(handle), std::forward_as_tuple(std::move(feature), handle));

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

  LOG_INFO("Destroying feature '{}' with handle {}", it->second.feature ? it->second.feature->GetName() : "unknown", feature_handle);
  if (it->second.feature)
  {
    m_renderGraph->RemoveFeature(it->second.feature.get());
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

const ToneMappingSettings& Renderer::GetToneMappingSettings() const
{
  return m_renderGraph->GetLinearColorSystem().GetToneMappingSettings();
}

void Renderer::UpdateToneMappingSettings(const ToneMappingSettings& newSettings) const
{
  m_renderGraph->GetLinearColorSystem().UpdateToneMappingSettings(newSettings);
}


