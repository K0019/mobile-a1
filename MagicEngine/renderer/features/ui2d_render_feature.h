#pragma once
#include "renderer/render_feature.h"
#include "renderer/render_graph.h"
#include "renderer/ui/ui_primitives.h"

struct Ui2DRenderParams
{
  bool enabled = false;
  ui::PrimitiveDrawList drawList;
};

class Ui2DRenderFeature final : public RenderFeatureBase<Ui2DRenderParams>
{
public:
  using Parameters = Ui2DRenderParams;

  Ui2DRenderFeature() = default;

  const char* GetName() const override
  {
    return "Ui2DRenderFeature";
  }

  void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

private:
  void RenderUi(const internal::ExecutionContext& context);

  void EnsurePipeline(const internal::ExecutionContext& context);

  vk::Holder<vk::ShaderModuleHandle> vertShader_;
  vk::Holder<vk::ShaderModuleHandle> fragShader_;
  vk::Holder<vk::RenderPipelineHandle> pipeline_;
  vk::Holder<vk::SamplerHandle> linearSampler_;
  vk::Holder<vk::SamplerHandle> nearestSampler_;
  vk::Holder<vk::SamplerHandle> textSampler_;
  bool resourcesCreated_ = false;
};
