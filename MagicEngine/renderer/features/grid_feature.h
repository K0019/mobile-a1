#pragma once
#include "renderer/gfx_interface.h"
#include "renderer/render_graph.h"

enum class Axis : uint8_t
{
  X = 0,
  Y = 1,
  Z = 2,
};

inline const char* AxisToString(const Axis e)
{
  switch (e)
  {
  case Axis::X:
    return "X";
  case Axis::Y:
    return "Y";
  case Axis::Z:
    return "Z";
  }
  return "";
}

namespace render_feature_internal
{
  struct Grid_Parameters
  {
    bool enabled = true;
    Axis axis = Axis::Y; // Default to Y (XZ plane)
    int majorGridDivisions = 10;
    vec3 originOffset = vec3(0.0f);
    uint32_t pipelineSamples = 1;
  };
}

class GridFeature final : public RenderFeatureBase<render_feature_internal::Grid_Parameters>
{
public:
  using Parameters = render_feature_internal::Grid_Parameters;

  ~GridFeature() override;

  [[nodiscard]] const char* GetName() const override;

  void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

  // Allow direct access to parameter buffers for immediate updates
  using RenderFeatureBase<render_feature_internal::Grid_Parameters>::param_;

private:
  void EnsurePipelineCreated(const internal::ExecutionContext& context);

  bool resourcesCreated_ = false;
  gfx::Holder<gfx::Pipeline> pipeline_;
};
