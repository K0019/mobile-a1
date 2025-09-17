#pragma once
#include <graphics/interface.h>
#include <graphics/render_graph.h>
#include <assets/core/asset_system.h>
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

      void SetupPasses(Render::internal::RenderPassBuilder& passBuilder) override;
    private:
      void RenderImGui(const Render::internal::ExecutionContext& context);
      void CopySceneForImGuiView(const Render::internal::ExecutionContext& context);
      void EnsurePipelineCreated(const Render::internal::ExecutionContext& context);

      vk::Holder<vk::ShaderModuleHandle> vertShader_;
      vk::Holder<vk::ShaderModuleHandle> fragShader_;
      vk::Holder<vk::RenderPipelineHandle> pipeline_;
      vk::Holder<vk::SamplerHandle> fontSampler_;
      bool resourcesCreated_ = false;
  };
} // namespace editor
