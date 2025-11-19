#pragma once
#include <cstdint>
#include <vector>
#include "graphics/interface.h"
#include "math/camera.h"

class LineCanvas3D
{
public:
  void clear() { lines_.clear(); }

  bool isEmpty() const { return lines_.empty(); }

  void line(const vec3& p1, const vec3& p2, const vec4& c);

  void plane(const vec3& orig, const vec3& v1, const vec3& v2, int n1, int n2, float s1, float s2, const vec4& color,
             const vec4& outlineColor);

  /*void box(const mat4& m, const BoundingBox& box, const vec4& color);*/
  void box(const mat4& m, const vec3& size, const vec4& color);

  void frustum(const mat4& camView, const mat4& camProj, const vec4& color);

  void setMatrix(const mat4& mvp) { mvp_ = mvp; }

  void render(vk::IContext& ctx, const vk::Framebuffer& desc, vk::ICommandBuffer& buf, uint32_t numSamples = 1);

private:
  mat4 mvp_ = mat4(1.0f);

  struct LineData
  {
    vec4 pos;
    vec4 color;
  };

  std::vector<LineData> lines_;
  vk::Holder<vk::ShaderModuleHandle> vert_;
  vk::Holder<vk::ShaderModuleHandle> frag_;
  vk::Holder<vk::RenderPipelineHandle> pipeline_;
  vk::Holder<vk::BufferHandle> linesBuffer_[3] = {};
  uint32_t pipelineSamples_ = 1;
  uint32_t currentBufferSize_[3] = {};
  uint32_t currentFrame_ = 0;
  static_assert(
    sizeof(linesBuffer_) / sizeof(linesBuffer_[0]) == sizeof(currentBufferSize_) / sizeof(currentBufferSize_[0]));
};
