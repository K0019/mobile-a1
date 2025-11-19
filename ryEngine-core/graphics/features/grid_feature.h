#pragma once
#include "graphics/interface.h"
#include "graphics/render_graph.h"

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
    Axis axis = Axis::Y; // Default to Y (XZ plane)
    int majorGridDivisions = 10;
    vec3 originOffset;
    uint32_t pipelineSamples;
  };
}

class GridFeature final : public RenderFeatureBase<render_feature_internal::Grid_Parameters>
{
public:
  using Parameters = render_feature_internal::Grid_Parameters;

  ~GridFeature() override;

  [[nodiscard]] const char* GetName() const override;

  void SetupPasses(internal::RenderPassBuilder& passBuilder) override;

private:
  void EnsurePipelineCreated(const internal::ExecutionContext& context);;

  uint32_t cachedSamples_ = 1;
  vk::Holder<vk::ShaderModuleHandle> vert_;
  vk::Holder<vk::ShaderModuleHandle> frag_;
  vk::Holder<vk::RenderPipelineHandle> pipeline_;
};
