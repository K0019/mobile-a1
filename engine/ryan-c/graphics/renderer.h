#pragma once
#include <array>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <variant>

#include "interface.h"
#include "i_renderer.h"
#include "renderer_resources.h"
#include "render_graph.h"

typedef struct GLFWwindow GLFWwindow;

class Renderer : public IRenderer
{
  public:
    Renderer(GLFWwindow* window, int width, int height);

    // Lifecycle - all called from main thread
    void startup();

    void shutdown();

    // Frame processing - all on main thread
    void beginFrame();

    void render(FrameData& frameData);

    void endFrame();

    void onWindowResized(int width, int height);

    // Feature system interface (simplified)
    void DestroyFeature(uint64_t feature_handle) override;

    void* GetFeatureParameterBlockPtr(uint64_t feature_handle) override;

    GPUBuffers& getGPUBuffers() { return *m_gpuBuffers; }

    const GPUBuffers& getGPUBuffers() const { return *m_gpuBuffers; }

    const ToneMappingSettings& GetToneMappingSettings() const;

    void UpdateToneMappingSettings(const ToneMappingSettings& newSettings) const;

    bool isWindowReadyForShow() const { return m_windowReadyForShow; }

    void AddTransientResourceObserver(internal::ITransientResourceObserver* observer) const;

    void RemoveTransientResourceObserver(internal::ITransientResourceObserver* observer) const;

  private:
    uint64_t DoCreateFeature(std::function<std::unique_ptr<IRenderFeature>()> factory) override;

    // Frame-in-flight management (simplified)
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

    struct PerFrameResources
    {
      ::FrameData frameData;
      // Each feature manages its own per-frame data via double-buffering
    };

    std::array<PerFrameResources, MAX_FRAMES_IN_FLIGHT> m_frameResources;
    std::array<vk::SubmitHandle, MAX_FRAMES_IN_FLIGHT> m_frameSubmitHandles;
    uint32_t m_currentFrame = 0;

    struct FeatureInfo
    {
      std::unique_ptr<IRenderFeature> feature;
      uint64_t handle;

      explicit FeatureInfo(std::unique_ptr<IRenderFeature> feat, uint64_t h) : feature(std::move(feat)), handle(h) {}
    };

    std::unordered_map<uint64_t, FeatureInfo> m_features;
    std::atomic<uint64_t> m_nextFeatureHandle{1};

    // Core systems
    std::unique_ptr<vk::IContext> m_vkContext;
    std::unique_ptr<::RenderGraph> m_renderGraph;
    std::unique_ptr<GPUBuffers> m_gpuBuffers;

    GLFWwindow* m_window;
    int m_windowWidth, m_windowHeight;

    int m_framesRendered = 0;
    bool m_windowReadyForShow = false;
    const int MIN_FRAMES_BEFORE_SHOW = 3;
};
