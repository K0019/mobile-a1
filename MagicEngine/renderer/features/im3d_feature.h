#pragma once
#include "renderer/gfx_interface.h"
#include "renderer/render_graph.h"
#include <im3d.h>
#include <vector>

namespace render_feature_internal
{
  struct Im3d_Parameters
  {
    bool enabled = true;
    uint32_t pipelineSamples = 1;
    // Raw Im3d vertex data per primitive type (filled by Im3dHelper::EndFrame)
    std::vector<Im3d::VertexData> pointVertices;
    std::vector<Im3d::VertexData> lineVertices;
    std::vector<Im3d::VertexData> triVertices;
  };
}

class Im3dRenderFeature final : public RenderFeatureBase<render_feature_internal::Im3d_Parameters>
{
public:
  using Parameters = render_feature_internal::Im3d_Parameters;

  ~Im3dRenderFeature() override;

  [[nodiscard]] const char* GetName() const override;

  void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

private:
  void EnsurePipelinesCreated(const internal::ExecutionContext& context);

  void ExecuteDrawPass(const internal::ExecutionContext& context);

  uint32_t cachedSamples_ = 1;
  // Pipelines
  gfx::Holder<gfx::Pipeline> pointPipeline_;
  gfx::Holder<gfx::Pipeline> linePipeline_;
  gfx::Holder<gfx::Pipeline> trianglePipeline_;
};
