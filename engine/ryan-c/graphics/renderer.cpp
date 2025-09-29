#include "renderer.h"

#include "asset_types.h"
#include "logging/log.h"
#include "logging/profiler.h"

Renderer::Renderer(GLFWwindow* window, int width, int height) : m_window(window), m_windowWidth(width), m_windowHeight(height)
{
  // Initialize frame submit handles to empty (invalid) state
  m_frameSubmitHandles.fill(vk::SubmitHandle{});

  // Initialize frame resources
  for (auto& frame : m_frameResources)
  {
    frame.frameData = {};
  }

  LOG_INFO("Renderer created (single-threaded) - {}x{}", width, height);
}

void Renderer::startup()
{
  PROFILER_FUNCTION();

  // Create Vulkan context on main thread (no threading issues)
  m_vkContext = vk::createVulkanContextWithSwapchain(m_window, m_windowWidth, m_windowHeight, {.vulkanVersion = vk::VulkanVersion_1_3, .terminateOnValidationError = false, .enableValidation = false, .swapchainRequestedColorSpace = vk::ColorSpace::SRGB_LINEAR,});

  if (!m_vkContext)
  {
    LOG_CRITICAL("Failed to create Vulkan context");
    throw std::runtime_error("Failed to create Vulkan context");
  }
  // Create GPU buffers (no upload response buffer needed)
  m_gpuBuffers = std::make_unique<GPUBuffers>(*m_vkContext,
                                              ResourceLimits::VERTEX_BUFFER_SIZE,
                                              // 40MB
                                              ResourceLimits::INDEX_BUFFER_SIZE,
                                              // 120MB  
                                              ResourceLimits::MATERIAL_BUFFER_SIZE);
  // Create render graph
  m_renderGraph = std::make_unique<RenderGraph>(*m_vkContext, *m_gpuBuffers);

  LOG_INFO("Renderer startup complete");
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
  frameData.frameNumber = static_cast<uint64_t>(m_framesRendered);
  m_frameResources[m_currentFrame].frameData = frameData;
  for (auto& info : m_features)
  {
    if (info.second.feature)
    {
      info.second.feature->SwapBuffersForGT_RT();
    }
  }
  // Get swapchain texture - VK wrapper handles acquire automatically
  const vk::TextureHandle currentTexture = m_vkContext->getCurrentSwapchainTexture();
  if (!currentTexture.valid())
  {
    return;
  }
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

  m_windowWidth = width;
  m_windowHeight = height;
  LOG_INFO("Window resized to {}x{}", width, height);
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


