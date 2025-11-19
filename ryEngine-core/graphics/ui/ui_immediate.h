#pragma once

#include <cstdint>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

#include "graphics/features/ui2d_render_feature.h"
#include "graphics/ui/ui_primitives.h"

class Renderer;
namespace Resource
{
  class ResourceManager;
}

namespace ui
{
  struct DrawOptions
  {
    vec4 clipRect{ 0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max() };
    bool hasClipRect = false;
    uint16_t layer = 0;
    FontHandle fontOverride;
  };

  class ImmediateGui
  {
    public:
      ImmediateGui(Renderer& renderer, uint64_t featureHandle, Resource::ResourceManager& resourceMngr);

      bool begin(FontHandle fontHandle = {});
      void setViewport(float width, float height);
      void setClipRect(const vec4& clipRect);
      void addSolidRect(const vec2& min, const vec2& max, const vec4& color, const DrawOptions& options = DrawOptions{});
      bool addLine(const vec2& p1, const vec2& p2, const vec4& color, float thickness = 1.0f, const DrawOptions& options = DrawOptions{});
      bool addPolyline(const vec2* points, size_t pointCount, bool closed, const vec4& color, float thickness = 1.0f, const DrawOptions& options = DrawOptions{});
      void addRectOutline(const vec2& min, const vec2& max, float thickness, const vec4& color, const DrawOptions& options = DrawOptions{});
      bool addTriangle(const vec2& p1, const vec2& p2, const vec2& p3, const vec4& color, float thickness = 1.0f, const DrawOptions& options = DrawOptions{});
      bool addTriangleFilled(const vec2& p1, const vec2& p2, const vec2& p3, const vec4& color, const DrawOptions& options = DrawOptions{});
      bool addCircle(const vec2& center, float radius, const vec4& color, int numSegments = 0, float thickness = 1.0f, const DrawOptions& options = DrawOptions{});
      bool addCircleFilled(const vec2& center, float radius, const vec4& color, int numSegments = 0, const DrawOptions& options = DrawOptions{});
      bool addNgon(const vec2& center, float radius, int numSegments, const vec4& color, float thickness = 1.0f, const DrawOptions& options = DrawOptions{});
      bool addNgonFilled(const vec2& center, float radius, int numSegments, const vec4& color, const DrawOptions& options = DrawOptions{});
      bool addImage(TextureHandle textureHandle,
                    const vec2& min,
                    const vec2& max,
                    const vec2& uvMin = vec2(0.0f),
                    const vec2& uvMax = vec2(1.0f),
                    const vec4& color = vec4(1.0f),
                    SamplerMode samplerMode = SamplerMode::Linear,
                    const DrawOptions& options = DrawOptions{});
      bool addImage(uint32_t textureHandle,
                    const vec2& min,
                    const vec2& max,
                    const vec2& uvMin = vec2(0.0f),
                    const vec2& uvMax = vec2(1.0f),
                    const vec4& color = vec4(1.0f),
                    SamplerMode samplerMode = SamplerMode::Linear,
                    const DrawOptions& options = DrawOptions{});
      bool addText(std::string_view text, TextLayoutDesc layout, const vec4& color, const DrawOptions& options = DrawOptions{});
      template <typename FirstArg, typename... Args, typename = std::enable_if_t<!std::is_same_v<std::decay_t<FirstArg>, DrawOptions>>>
      bool addText(std::string_view fmtStr,
                   TextLayoutDesc layout,
                   const vec4& color,
                   FirstArg&& firstArg,
                   Args&&... args)
      {
        return addText(fmtStr,
                       layout,
                       color,
                       DrawOptions{},
                       std::forward<FirstArg>(firstArg),
                       std::forward<Args>(args)...);
      }

      template <typename... Args, typename = std::enable_if_t<(sizeof...(Args) > 0)>>
      bool addText(std::string_view fmtStr,
                   TextLayoutDesc layout,
                   const vec4& color,
                   const DrawOptions& options,
                   Args&&... args)
      {
        // fmt::make_format_args stores references internally, so pass named lvalues.
        return addText(fmt::vformat(fmtStr, fmt::make_format_args(args...)), layout, color, options);
      }

      vec2 measureText(std::string_view text, TextLayoutDesc layout, FontHandle fontOverride = {}) const;
      PrimitiveDrawList& drawList();
      void end();

    private:
      vec4 resolveClipRect(const DrawOptions& options) const;
      uint16_t resolveLayer(const DrawOptions& options) const;
      FontHandle resolveFont(FontHandle font) const;

      Renderer& renderer_;
      uint64_t featureHandle_;
      Resource::ResourceManager& resourceMngr_;
      Ui2DRenderFeature::Parameters* params_ = nullptr;
      PrimitiveDrawList* drawList_ = nullptr;
      FontHandle fontHandle_;
      vec4 frameClipRect_{0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
      bool valid_ = false;
      bool began_ = false;
  };
} // namespace ui
