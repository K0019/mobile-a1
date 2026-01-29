#pragma once
#include <renderer/gfx_interface.h>
#include <renderer/render_graph.h>
// Forward declaration
struct ImDrawData;

namespace editor
{
  /**
   * Parameters for ImGui render feature
   */
  struct ImGuiRenderParams
  {
    bool enabled = true;
    bool hasNewFrame = false;
  };

  class ImGuiRenderFeature final : public RenderFeatureBase<ImGuiRenderParams>
  {
  public:
    using Parameters = ImGuiRenderParams;

    explicit ImGuiRenderFeature() = default;

    ~ImGuiRenderFeature() override = default;

    const char* GetName() const override
    {
      return "ImGuiRenderFeature";
    }

    void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

  private:
    void RenderImGui(const internal::ExecutionContext& context);

    void CopySceneForImGuiView(const internal::ExecutionContext& context);

    void EnsurePipelineCreated(const internal::ExecutionContext& context);

    gfx::Holder<gfx::Pipeline> pipeline_;
    gfx::Holder<gfx::Sampler> fontSampler_;
    bool resourcesCreated_ = false;
  };
} // namespace editor
