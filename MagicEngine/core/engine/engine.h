// core/engine/engine.h
#pragma once
#include <atomic>
#include "renderer/gfx_renderer.h"
#include "renderer/frame_data.h"
#include "core/platform/platform.h"
#include "core_utils/clock.h"
#include "logging/log.h"
#include "logging/profiler.h"
#include "resource/resource_manager.h"

struct Context
{
  GfxRenderer* renderer = nullptr;
  Resource::ResourceManager* resourceMngr = nullptr;
};

template <typename T>concept App = requires(T& app, Context& ctx, RenderFrameData& frame)
{
  { app.Initialize(ctx) } -> std::same_as<void>; { app.Update(ctx, frame) } -> std::same_as<void>; {
    app.Shutdown(ctx)
  } -> std::same_as<void>;
};

template <App AppType>
class Engine
{
public:
  Engine();

  ~Engine();

  Engine(const Engine&) = delete;

  Engine& operator=(const Engine&) = delete;

  Engine(Engine&&) = delete;

  Engine& operator=(Engine&&) = delete;

  void Initialize();

  bool ExecuteFrame();

  void Shutdown();

  Context context;

private:
  AppType m_application;
  Core::Clock m_clock;
  Resource::ResourceManager m_assetSystem;
  GfxRenderer m_renderer;
  RenderFrameData m_currentFrameData;
  uint64_t m_frameCounter = 0;
  std::atomic<bool> m_initialized{false};
  std::atomic<bool> m_surfaceValid{false};

  void InitializeCoreSystems();

  void CleanupCoreSystems();

  void OnLifecycleChange(Core::AppState oldState, Core::AppState newState);

  void OnSurfaceCreated();

  void OnSurfaceDestroyed();

  void OnLowMemory();
};

template <App AppType>
Engine<AppType>::Engine() = default;

template <App AppType>
Engine<AppType>::~Engine()
{
  if (m_initialized.load())
  {
    CleanupCoreSystems();
  }
}

template <App AppType>
void Engine<AppType>::Initialize()
{
  // Register lifecycle callbacks
  Core::Lifecycle().onStateChange = [this](Core::AppState old, Core::AppState newState)
  {
    OnLifecycleChange(old, newState);
  };
  Core::Lifecycle().onLowMemory = [this]()
  {
    OnLowMemory();
  };
  Core::Display().onResize = [this](int width, int height)
  {
    // Skip resize when window is minimized (width/height = 0)
    if (width > 0 && height > 0 && m_surfaceValid.load())
    {
      m_renderer.onResize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }
  };
  // Android-specific callbacks
#if defined(__ANDROID__)
  Core::Display().onSurfaceCreated = [this]()
  {
    OnSurfaceCreated();
  }; Core::Display().onSurfaceDestroyed = [this]()
  {
    OnSurfaceDestroyed();
  };
#endif
  // Initialize core systems (headless)
  InitializeCoreSystems();
  // Desktop: Surface available immediately
#if !defined(__ANDROID__)
  if (Core::Display().IsValid())
  {
    OnSurfaceCreated();
    m_application.Initialize(context);
    m_initialized.store(true);
  }
#endif
  // Android: Wait for onSurfaceCreated callback
  LOG_INFO("Engine initialized");
}

template <App AppType>
bool Engine<AppType>::ExecuteFrame()
{
  PROFILER_ZONE("EngineFrame", PROFILER_COLOR_CPU_UPDATE);
    Core::Input().Update();
    Core::Platform::Get().ProcessEvents();
    if (Core::Platform::Get().GetLifecycle().ShouldQuit())
    {
      return false;
    }
    if (Core::Platform::Get().GetLifecycle().GetState() != Core::AppState::Running)
    {
      return true;
    }
    if (!Core::Platform::Get().GetDisplay().IsValid())
    {
      return true;
    }
    // Delayed initialization for Android
    if (!m_initialized.load())
    {
      m_application.Initialize(context);
      m_initialized.store(true);
    }
    m_clock.Tick();
    float deltaTime = m_clock.GetDeltaTime();
    int width = Core::Platform::Get().GetDisplay().GetWidth();
    int height = Core::Platform::Get().GetDisplay().GetHeight();
    if (width > 0 && height > 0 && m_surfaceValid.load())
    {
      if (!m_renderer.beginFrame())
      {
        // Swapchain acquisition failed (e.g., window minimized) - skip this frame
        return true;
      }

      // Update RenderFrameData with current frame info
      m_currentFrameData.frameInfo.frameNumber = m_frameCounter;
      m_currentFrameData.frameInfo.deltaTime = deltaTime;
      m_currentFrameData.surface.presentWidth = static_cast<uint32_t>(width);
      m_currentFrameData.surface.presentHeight = static_cast<uint32_t>(height);
      m_currentFrameData.surface.renderWidth = static_cast<uint32_t>(width);
      m_currentFrameData.surface.renderHeight = static_cast<uint32_t>(height);

      // Ensure at least one view exists for the primary camera
      // Application.Update should populate camera matrices via SetViewCamera -> GraphicsMain::frameData
      FrameData& primaryView = EnsureView(m_currentFrameData, 0);
      primaryView.viewportWidth = static_cast<float>(width);
      primaryView.viewportHeight = static_cast<float>(height);
      primaryView.screenWidth = static_cast<uint32_t>(width);
      primaryView.screenHeight = static_cast<uint32_t>(height);
      primaryView.deltaTime = deltaTime;
      primaryView.frameNumber = m_frameCounter;

      {
        PROFILER_ZONE("Application Update", PROFILER_COLOR_CPU_LOGIC);
          m_application.Update(context, m_currentFrameData);
        PROFILER_ZONE_END()
      }
      m_assetSystem.pollAsyncUploads();
      {
        PROFILER_ZONE("Render", PROFILER_COLOR_CMD_DRAW);
          m_renderer.render(m_currentFrameData);
        PROFILER_ZONE_END()
      }
      m_renderer.endFrame();
      ++m_frameCounter;
#if !defined(__ANDROID__)
      static bool m_windowShown = false;
      if (!m_windowShown)
      {
        if (m_renderer.isWindowReadyForShow())
        {
          Core::Platform::Get().GetDisplay().Show();
          m_windowShown = true;
        }
      }
#endif
    }
  PROFILER_ZONE_END()
  return true;
}

template <App AppType>
void Engine<AppType>::Shutdown()
{
  if (m_initialized.load())
  {
    m_application.Shutdown(context);
    m_initialized.store(false);
    m_application = {};
  }
  CleanupCoreSystems();
  LOG_INFO("Engine shut down");
}

template <App AppType>
void Engine<AppType>::InitializeCoreSystems()
{
  m_assetSystem.initialize(&context);
  context.resourceMngr = &m_assetSystem;
  // GfxRenderer is constructed but not initialized yet - needs window
  // On desktop, initialize() is called in OnSurfaceCreated()
  // On Android, initialize() is called in handleSurfaceCreated()
  context.renderer = &m_renderer;
  m_renderer.setResourceManager(&m_assetSystem);
  m_assetSystem.postRendererInitialize();
  LOG_INFO("Core systems initialized (headless)");
}

template <App AppType>
void Engine<AppType>::CleanupCoreSystems()
{
  // Stop upload thread first — it references the renderer and mesh storage
  m_assetSystem.shutdown();
  m_renderer.setResourceManager(nullptr);
  m_renderer.shutdown();
  context.renderer = nullptr;
  context.resourceMngr = nullptr;
  LOG_INFO("Core systems cleaned up");
}

template <App AppType>
void Engine<AppType>::OnLifecycleChange([[maybe_unused]] Core::AppState oldState, [[maybe_unused]] Core::AppState newState)
{
  LOG_INFO("Lifecycle change: {} -> {}", static_cast<int>(oldState), static_cast<int>(newState));
}

template <App AppType>
void Engine<AppType>::OnSurfaceCreated()
{
  LOG_INFO("Surface created callback");
  void* nativeWindow = Core::Display().GetVulkanWindowHandle();
  uint32_t width = Core::Display().GetWidth();
  uint32_t height = Core::Display().GetHeight();

  // Initialize or recreate surface
  if (!m_renderer.isInitialized()) {
    // First time initialization
    if (!m_renderer.initialize(nativeWindow, width, height)) {
      LOG_ERROR("Failed to initialize GfxRenderer");
      return;
    }
    // Start async upload thread now that hina-vk is initialized
    m_assetSystem.startUploadThread();
  } else {
    // Recreate surface after lifecycle event (Android)
    m_renderer.handleSurfaceCreated(nativeWindow);
  }
  m_surfaceValid.store(true);

  // Android: Initialize app on first surface creation
#if defined(__ANDROID__)
  if (!m_initialized.load())
  {
    m_application.Initialize(context);
    m_initialized.store(true);
  }
#endif
}

template <App AppType>
void Engine<AppType>::OnSurfaceDestroyed()
{
  LOG_INFO("Surface destroyed callback");
  m_surfaceValid.store(false);
  // Destroy surface and swapchain
  m_renderer.handleSurfaceDestroyed();
}

template <App AppType>
void Engine<AppType>::OnLowMemory()
{
  LOG_WARNING("Low memory warning received");
}
