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
    // CPU-built packed buffer (single upload)
    std::vector<uint8_t> packed;
    // Offsets are byte offsets into 'packed'; counts are element counts
    uint32_t offPoints = 0, numPoints = 0; // PointGPU elements
    uint32_t offLines = 0, numLines = 0; // LineGPU elements
    uint32_t offTris = 0, numTris = 0; // TriGPU elements
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
  // Shader modules
  gfx::Holder<gfx::Shader> pointVS_;
  gfx::Holder<gfx::Shader> pointFS_;
  gfx::Holder<gfx::Shader> lineVS_;
  gfx::Holder<gfx::Shader> lineFS_;
  gfx::Holder<gfx::Shader> triVS_;
  gfx::Holder<gfx::Shader> triFS_;
  // Pipelines (fixed topology)
  gfx::Holder<gfx::Pipeline> pointPipeline_;
  gfx::Holder<gfx::Pipeline> linePipeline_;
  gfx::Holder<gfx::Pipeline> trianglePipeline_;
};
