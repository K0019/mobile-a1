#pragma once
#include "renderer/render_feature.h"
#include "renderer/render_graph.h"
#include "renderer/ui/ui_primitives.h"

struct Ui2DRenderParams
{
  bool enabled = false;
  ui::PrimitiveDrawList drawList;
  // UI viewport dimensions - dynamic based on window aspect ratio
  // Height is always 1080 (reference), width scales with aspect ratio
  float viewportWidth = 1920.0f;
  float viewportHeight = 1080.0f;
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

  gfx::Holder<gfx::Pipeline> pipeline_;
  bool resourcesCreated_ = false;
};
