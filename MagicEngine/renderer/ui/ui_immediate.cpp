#include "renderer/ui/ui_immediate.h"
#include "renderer/gfx_renderer.h"
#include "resource/resource_manager.h"

namespace ui
{
  ImmediateGui::ImmediateGui(GfxRenderer& renderer, uint64_t featureHandle, Resource::ResourceManager& resourceMngr)
    : renderer_(renderer), featureHandle_(featureHandle), resourceMngr_(resourceMngr)
  {
  }

  bool ImmediateGui::begin(FontHandle fontHandle)
  {
    fontHandle_ = resolveFont(fontHandle);
    began_ = true;
    valid_ = false;
    params_ = nullptr;
    drawList_ = nullptr;
    if (!fontHandle_.isValid() || featureHandle_ == 0)
    {
      return false;
    }
    params_ = static_cast<Ui2DRenderFeature::Parameters*>(renderer_.GetFeatureParameterBlockPtr(featureHandle_));
    if (!params_)
    {
      return false;
    }
    params_->enabled = true;
    params_->drawList.clear();
    drawList_ = &params_->drawList;
    valid_ = ApplySolidFillFallback(*drawList_, resourceMngr_, fontHandle_);
    if (!valid_)
    {
      params_->enabled = false;
      drawList_ = nullptr;
    }
    frameClipRect_ = vec4(0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    return valid_;
  }

  void ImmediateGui::setViewport(float width, float height)
  {
    frameClipRect_ = vec4(0.0f, 0.0f, width, height);
  }

  void ImmediateGui::setClipRect(const vec4& clipRect)
  {
    frameClipRect_ = clipRect;
  }

  void ImmediateGui::addSolidRect(const vec2& min, const vec2& max, const vec4& color, const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return;
    AddSolidRect(*drawList_, min, max, PackColor(color), resolveClipRect(options), resolveLayer(options));
  }

  bool ImmediateGui::addLine(const vec2& p1, const vec2& p2, const vec4& color, float thickness,
                             const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddLine(*drawList_, p1, p2, PackColor(color), thickness, resolveClipRect(options), resolveLayer(options));
  }

  bool ImmediateGui::addPolyline(const vec2* points, size_t pointCount, bool closed, const vec4& color, float thickness,
                                 const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddPolyline(*drawList_, points, pointCount, closed, PackColor(color), thickness, resolveClipRect(options),
                       resolveLayer(options));
  }

  void ImmediateGui::addRectOutline(const vec2& min, const vec2& max, float thickness, const vec4& color,
                                    const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return;
    AddRectOutline(*drawList_, min, max, thickness, PackColor(color), resolveClipRect(options), resolveLayer(options));
  }

  bool ImmediateGui::addTriangle(const vec2& p1, const vec2& p2, const vec2& p3, const vec4& color, float thickness,
                                 const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddTriangle(*drawList_, p1, p2, p3, PackColor(color), thickness, resolveClipRect(options),
                       resolveLayer(options));
  }

  bool ImmediateGui::addTriangleFilled(const vec2& p1, const vec2& p2, const vec2& p3, const vec4& color,
                                       const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddTriangleFilled(*drawList_, p1, p2, p3, PackColor(color), resolveClipRect(options), resolveLayer(options));
  }

  bool ImmediateGui::addCircle(const vec2& center, float radius, const vec4& color, int numSegments, float thickness,
                               const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddCircle(*drawList_, center, radius, PackColor(color), numSegments, thickness, resolveClipRect(options),
                     resolveLayer(options));
  }

  bool ImmediateGui::addCircleFilled(const vec2& center, float radius, const vec4& color, int numSegments,
                                     const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddCircleFilled(*drawList_, center, radius, PackColor(color), numSegments, resolveClipRect(options),
                           resolveLayer(options));
  }

  bool ImmediateGui::addNgon(const vec2& center, float radius, int numSegments, const vec4& color, float thickness,
                             const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddNgon(*drawList_, center, radius, PackColor(color), numSegments, thickness, resolveClipRect(options),
                   resolveLayer(options));
  }

  bool ImmediateGui::addNgonFilled(const vec2& center, float radius, int numSegments, const vec4& color,
                                   const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddNgonFilled(*drawList_, center, radius, PackColor(color), numSegments, resolveClipRect(options),
                         resolveLayer(options));
  }

  bool ImmediateGui::addImage(TextureHandle textureHandle, const vec2& min, const vec2& max, const vec2& uvMin,
                              const vec2& uvMax, const vec4& color, SamplerMode samplerMode, const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddImage(resourceMngr_, *drawList_, textureHandle, min, max, uvMin, uvMax, PackColor(color),
                    resolveClipRect(options), samplerMode, resolveLayer(options));
  }

  bool ImmediateGui::addImage(uint32_t textureHandle, const vec2& min, const vec2& max, const vec2& uvMin,
                              const vec2& uvMax, const vec4& color, SamplerMode samplerMode, const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    return AddImage(resourceMngr_, *drawList_, textureHandle, min, max, uvMin, uvMax, PackColor(color),
                    resolveClipRect(options), samplerMode, resolveLayer(options));
  }

  bool ImmediateGui::addText(std::string_view text, TextLayoutDesc layout, const vec4& color,
                             const DrawOptions& options)
  {
    if (!valid_ || !drawList_) return false;
    if (layout.clipRect.z == std::numeric_limits<float>::max())
    {
      layout.clipRect = resolveClipRect(options);
    }
    FontHandle targetFont = resolveFont(options.fontOverride.isValid() ? options.fontOverride : fontHandle_);
    if (!targetFont.isValid()) return false;
    return AddText(resourceMngr_, *drawList_, targetFont, text, layout, PackColor(color), resolveLayer(options));
  }

  vec2 ImmediateGui::measureText(std::string_view text, TextLayoutDesc layout, FontHandle fontOverride) const
  {
    FontHandle targetFont = resolveFont(fontOverride.isValid() ? fontOverride : fontHandle_);
    if (!targetFont.isValid()) return vec2(0.0f);
    if (layout.clipRect.z == std::numeric_limits<float>::max())
    {
      layout.clipRect = frameClipRect_;
    }
    return MeasureText(resourceMngr_, targetFont, text, layout);
  }

  PrimitiveDrawList& ImmediateGui::drawList()
  {
    return *drawList_;
  }

  void ImmediateGui::end()
  {
    if (!began_) return;
    if (params_ && !valid_)
    {
      params_->enabled = false;
    }
    began_ = false;
    params_ = nullptr;
    drawList_ = nullptr;
    valid_ = false;
  }

  vec4 ImmediateGui::resolveClipRect(const DrawOptions& options) const
  {
    return options.hasClipRect ? options.clipRect : frameClipRect_;
  }

  uint16_t ImmediateGui::resolveLayer(const DrawOptions& options) const
  {
    return options.layer;
  }

  FontHandle ImmediateGui::resolveFont(FontHandle font) const
  {
    if (font.isValid()) return font;
    return resourceMngr_.getDefaultUIFont();
  }
} // namespace ui
