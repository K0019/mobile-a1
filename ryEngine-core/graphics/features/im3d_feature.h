#pragma once
#include "graphics/interface.h"
#include "graphics/render_graph.h"
#include "im3d.h"
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
  vk::Holder<vk::ShaderModuleHandle> pointVS_;
  vk::Holder<vk::ShaderModuleHandle> pointFS_;
  vk::Holder<vk::ShaderModuleHandle> lineVS_;
  vk::Holder<vk::ShaderModuleHandle> lineFS_;
  vk::Holder<vk::ShaderModuleHandle> triVS_;
  vk::Holder<vk::ShaderModuleHandle> triFS_;
  // Pipelines (fixed topology)
  vk::Holder<vk::RenderPipelineHandle> pointPipeline_;
  vk::Holder<vk::RenderPipelineHandle> linePipeline_;
  vk::Holder<vk::RenderPipelineHandle> trianglePipeline_;
};
