#include "graphics/ui/ui_primitives.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <limits>
#include <glm/gtc/epsilon.hpp>
#include "resource/resource_manager.h"
#include "resource/resource_manager.h"
#include "resource/resource_types.h"

namespace
{
  constexpr uint32_t kInvalidTexture = std::numeric_limits<uint32_t>::max();
  constexpr float kTwoPi = 6.28318530718f;

  uint32_t decodeUtf8(std::string_view text, size_t& index)
  {
    if (index >= text.size()) return 0;
    const unsigned char c = static_cast<unsigned char>(text[index]);
    uint32_t codepoint = 0;
    size_t numBytes = 0;
    if ((c & 0x80u) == 0)
    {
      codepoint = c;
      numBytes = 1;
    }
    else if ((c & 0xE0u) == 0xC0u)
    {
      codepoint = c & 0x1Fu;
      numBytes = 2;
    }
    else if ((c & 0xF0u) == 0xE0u)
    {
      codepoint = c & 0x0Fu;
      numBytes = 3;
    }
    else if ((c & 0xF8u) == 0xF0u)
    {
      codepoint = c & 0x07u;
      numBytes = 4;
    }
    else
    {
      index++;
      return '?';
    }
    if (index + numBytes > text.size())
    {
      index = text.size();
      return '?';
    }
    for (size_t i = 1; i < numBytes; ++i)
    {
      codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[index + i]) & 0x3Fu);
    }
    index += numBytes;
    return codepoint;
  }

  bool clipRectsEqual(const vec4& a, const vec4& b)
  {
    constexpr float kEpsilon = 1e-4f;
    return glm::all(glm::epsilonEqual(a, b, kEpsilon));
  }

  ui::PrimitiveDrawCommand& AcquireOrMergeCommand(ui::PrimitiveDrawList& list, uint32_t textureId,
                                                  uint32_t samplerIndex, const vec4& clipRect, uint16_t layer,
                                                  uint32_t indexOffset, int32_t vertexOffset)
  {
    if (!list.commands.empty())
    {
      ui::PrimitiveDrawCommand& last = list.commands.back();
      if (last.textureId == textureId && last.samplerIndex == samplerIndex && clipRectsEqual(last.clipRect, clipRect) &&
        last.layer == layer)
      {
        return last;
      }
    }
    ui::PrimitiveDrawCommand cmd;
    cmd.indexOffset = indexOffset;
    cmd.vertexOffset = vertexOffset;
    cmd.clipRect = clipRect;
    cmd.textureId = textureId;
    cmd.samplerIndex = samplerIndex;
    cmd.layer = layer;
    cmd.sortOrder = list.commandOrderCounter++;
    list.commands.push_back(cmd);
    return list.commands.back();
  }

  bool pushColoredQuad(ui::PrimitiveDrawList& list, const vec2 positions[4], uint32_t color, const vec4& clipRect,
                       uint32_t samplerIndex, uint16_t layer)
  {
    if (!list.hasSolidFillFallback()) return false;
    const int32_t baseVertex = static_cast<int32_t>(list.vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(list.indices.size());
    ui::PrimitiveDrawCommand& cmd = AcquireOrMergeCommand(list, list.solidFillTextureId, samplerIndex, clipRect, layer,
                                                          baseIndex, baseVertex);
    list.vertices.reserve(list.vertices.size() + 4);
    for (int i = 0; i < 4; ++i)
    {
      list.vertices.push_back({positions[i].x, positions[i].y, list.whitePixelUV.x, list.whitePixelUV.y, color});
    }
    const uint32_t base = static_cast<uint32_t>(baseVertex);
    list.indices.reserve(list.indices.size() + 6);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 1);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 3);
    cmd.indexCount = static_cast<uint32_t>(list.indices.size()) - cmd.indexOffset;
    return true;
  }

  bool addConvexFill(ui::PrimitiveDrawList& list, const vec2* points, size_t count, uint32_t color,
                     const vec4& clipRect, uint32_t samplerIndex, uint16_t layer)
  {
    if (count < 3 || !list.hasSolidFillFallback()) return false;
    const int32_t baseVertex = static_cast<int32_t>(list.vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(list.indices.size());
    ui::PrimitiveDrawCommand& cmd = AcquireOrMergeCommand(list, list.solidFillTextureId, samplerIndex, clipRect, layer,
                                                          baseIndex, baseVertex);
    list.vertices.reserve(list.vertices.size() + count);
    for (size_t i = 0; i < count; ++i)
    {
      list.vertices.push_back({points[i].x, points[i].y, list.whitePixelUV.x, list.whitePixelUV.y, color});
    }
    const uint32_t base = static_cast<uint32_t>(baseVertex);
    list.indices.reserve(list.indices.size() + (count - 2) * 3);
    for (size_t i = 1; i + 1 < count; ++i)
    {
      list.indices.push_back(base + 0);
      list.indices.push_back(base + static_cast<uint32_t>(i));
      list.indices.push_back(base + static_cast<uint32_t>(i + 1));
    }
    cmd.indexCount = static_cast<uint32_t>(list.indices.size()) - cmd.indexOffset;
    return true;
  }

  struct GlyphPlacement
  {
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    const Resource::FontGlyph* glyph = nullptr;
  };

  template <typename Callback>
  bool IterateGlyphPlacements(const Resource::FontCPUData& fontCpu, std::string_view text,
                              const ui::TextLayoutDesc& layout, Callback&& callback)
  {
    if (text.empty()) return false;
    const float lineAdvance = (fontCpu.ascent - fontCpu.descent + fontCpu.lineGap) * layout.lineSpacing;
    const int oversample = (std::max)(1, fontCpu.buildSettings.oversample);
    const float invOversample = 1.0f / static_cast<float>(oversample);
    const bool snapToPixel = layout.useFontSnapSetting ? fontCpu.buildSettings.snapToPixel : layout.snapToPixel;
    const float roundedAscent = std::floor(fontCpu.ascent + 0.5f);
    const float baseline = layout.origin.y + (layout.originIsBaseline ? 0.0f : roundedAscent);
    vec2 cursor{layout.origin.x, baseline};
    size_t cursorIndex = 0;
    bool emittedGlyph = false;
    uint32_t prevCodepoint = 0;
    bool hasPrevCodepoint = false;
    auto snapHorizontal = [&](float& x0, float& x1)
    {
      if (snapToPixel)
      {
        x0 = std::floor(x0 + 0.5f);
        x1 = std::floor(x1 + 0.5f);
      }
    };
    while (cursorIndex < text.size())
    {
      uint32_t codepoint = decodeUtf8(text, cursorIndex);
      if (codepoint == '\n')
      {
        cursor.x = layout.origin.x;
        cursor.y += lineAdvance;
        hasPrevCodepoint = false;
        continue;
      }
      if (codepoint == '\r')
      {
        continue;
      }
      const Resource::FontGlyph* glyph = fontCpu.findGlyph(codepoint);
      if (!glyph)
      {
        glyph = fontCpu.findGlyph(fontCpu.buildSettings.fallbackCodepoint);
        if (!glyph) continue;
      }
      if (hasPrevCodepoint)
      {
        cursor.x += fontCpu.getKerning(prevCodepoint, codepoint);
      }
      float x0 = cursor.x + glyph->bearingPx.x;
      float y0 = cursor.y + glyph->bearingPx.y;
      const float width = glyph->sizePx.x * invOversample;
      const float height = glyph->sizePx.y * invOversample;
      float x1 = x0 + width;
      float y1 = y0 + height;
      snapHorizontal(x0, x1);
      if (layout.wrapWidth > 0.0f && (x1 - layout.origin.x) > layout.wrapWidth && cursor.x > layout.origin.x)
      {
        cursor.x = layout.origin.x;
        cursor.y += lineAdvance;
        x0 = cursor.x + glyph->bearingPx.x;
        y0 = cursor.y + glyph->bearingPx.y;
        x1 = x0 + width;
        y1 = y0 + height;
        snapHorizontal(x0, x1);
      }
      GlyphPlacement placement{x0, y0, x1, y1, glyph};
      callback(placement);
      emittedGlyph = true;
      cursor.x += glyph->advancePx + layout.letterSpacing;
      prevCodepoint = codepoint;
      hasPrevCodepoint = true;
    }
    return emittedGlyph;
  }
} // namespace
namespace ui
{
  bool ApplySolidFillFallback(PrimitiveDrawList& list, Resource::ResourceManager& resources, FontHandle font)
  {
    if (!font.isValid()) return false;
    const auto* fontHot = resources.getFont(font);
    if (!fontHot) return false;
    list.setSolidFillFallback(fontHot->bindlessIndex, fontHot->cpuData.whitePixelUV);
    return true;
  }

  uint32_t PackColor(const vec4& color)
  {
    const auto clamp = [](float v)
    {
      return static_cast<uint32_t>(std::round(std::clamp(v, 0.0f, 1.0f) * 255.0f));
    };
    const uint32_t r = clamp(color.r);
    const uint32_t g = clamp(color.g);
    const uint32_t b = clamp(color.b);
    const uint32_t a = clamp(color.a);
    return (a << 24u) | (b << 16u) | (g << 8u) | r;
  }

  void AddSolidRect(PrimitiveDrawList& list, const vec2& min, const vec2& max, uint32_t color, const vec4& clipRect,
                    uint16_t layer)
  {
    vec2 positions[4] = {{min.x, min.y}, {max.x, min.y}, {max.x, max.y}, {min.x, max.y}};
    pushColoredQuad(list, positions, color, clipRect, SamplerModeToIndex(SamplerMode::Linear), layer);
  }

  bool AddLine(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, uint32_t color, float thickness,
               const vec4& clipRect, uint16_t layer)
  {
    if (thickness <= 0.0f) return false;
    vec2 delta = p2 - p1;
    const float lenSq = glm::dot(delta, delta);
    if (lenSq <= std::numeric_limits<float>::epsilon()) return false;
    const float invLen = 1.0f / std::sqrt(lenSq);
    vec2 normal = vec2(-delta.y, delta.x) * invLen;
    normal *= (thickness * 0.5f);
    vec2 positions[4] = {p1 - normal, p1 + normal, p2 + normal, p2 - normal};
    return pushColoredQuad(list, positions, color, clipRect, SamplerModeToIndex(SamplerMode::Linear), layer);
  }

  bool AddPolyline(PrimitiveDrawList& list, const vec2* points, size_t pointCount, bool closed, uint32_t color,
                   float thickness, const vec4& clipRect, uint16_t layer)
  {
    if (pointCount < 2) return false;
    bool any = false;
    const size_t segments = closed ? pointCount : pointCount - 1;
    for (size_t i = 0; i < segments; ++i)
    {
      const vec2& p0 = points[i];
      const vec2& p1 = points[(i + 1) % pointCount];
      any |= AddLine(list, p0, p1, color, thickness, clipRect, layer);
    }
    return any;
  }

  void AddRectOutline(PrimitiveDrawList& list, const vec2& min, const vec2& max, float thickness, uint32_t color,
                      const vec4& clipRect, uint16_t layer)
  {
    if (thickness <= 0.0f || min.x >= max.x || min.y >= max.y) return;
    const float clamped = std::min(thickness, std::min(max.x - min.x, max.y - min.y) * 0.5f);
    const vec2 topMin = min;
    const vec2 topMax = vec2(max.x, min.y + clamped);
    const vec2 bottomMin = vec2(min.x, max.y - clamped);
    const vec2 bottomMax = max;
    const vec2 leftMin = vec2(min.x, min.y + clamped);
    const vec2 leftMax = vec2(min.x + clamped, max.y - clamped);
    const vec2 rightMin = vec2(max.x - clamped, min.y + clamped);
    const vec2 rightMax = vec2(max.x, max.y - clamped);
    AddSolidRect(list, topMin, topMax, color, clipRect, layer);
    AddSolidRect(list, bottomMin, bottomMax, color, clipRect, layer);
    AddSolidRect(list, leftMin, leftMax, color, clipRect, layer);
    AddSolidRect(list, rightMin, rightMax, color, clipRect, layer);
  }

  bool AddTriangle(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, const vec2& p3, uint32_t color,
                   float thickness, const vec4& clipRect, uint16_t layer)
  {
    const vec2 points[3] = {p1, p2, p3};
    return AddPolyline(list, points, 3, true, color, thickness, clipRect, layer);
  }

  bool AddTriangleFilled(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, const vec2& p3, uint32_t color,
                         const vec4& clipRect, uint16_t layer)
  {
    const vec2 points[3] = {p1, p2, p3};
    return addConvexFill(list, points, 3, color, clipRect, SamplerModeToIndex(SamplerMode::Linear), layer);
  }

  bool AddCircle(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                 float thickness, const vec4& clipRect, uint16_t layer)
  {
    if (radius <= 0.0f) return false;
    int segments = numSegments > 0 ? numSegments : static_cast<int>(std::max(12.0f, radius));
    segments = std::max(segments, 3);
    std::vector<vec2> points;
    points.reserve(static_cast<size_t>(segments));
    const float step = kTwoPi / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
      const float angle = step * static_cast<float>(i);
      points.emplace_back(center + vec2(std::cos(angle), std::sin(angle)) * radius);
    }
    return AddPolyline(list, points.data(), points.size(), true, color, thickness, clipRect, layer);
  }

  bool AddCircleFilled(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                       const vec4& clipRect, uint16_t layer)
  {
    if (radius <= 0.0f) return false;
    int segments = numSegments > 0 ? numSegments : static_cast<int>(std::max(12.0f, radius));
    segments = std::max(segments, 3);
    std::vector<vec2> points;
    points.reserve(static_cast<size_t>(segments));
    const float step = kTwoPi / static_cast<float>(segments);
    for (int i = 0; i < segments; ++i)
    {
      const float angle = step * static_cast<float>(i);
      points.emplace_back(center + vec2(std::cos(angle), std::sin(angle)) * radius);
    }
    return addConvexFill(list, points.data(), points.size(), color, clipRect, SamplerModeToIndex(SamplerMode::Linear),
                         layer);
  }

  bool AddNgon(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
               float thickness, const vec4& clipRect, uint16_t layer)
  {
    if (radius <= 0.0f || numSegments < 3) return false;
    std::vector<vec2> points;
    points.reserve(static_cast<size_t>(numSegments));
    const float step = kTwoPi / static_cast<float>(numSegments);
    for (int i = 0; i < numSegments; ++i)
    {
      const float angle = step * static_cast<float>(i);
      points.emplace_back(center + vec2(std::cos(angle), std::sin(angle)) * radius);
    }
    return AddPolyline(list, points.data(), points.size(), true, color, thickness, clipRect, layer);
  }

  bool AddNgonFilled(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                     const vec4& clipRect, uint16_t layer)
  {
    if (radius <= 0.0f || numSegments < 3) return false;
    std::vector<vec2> points;
    points.reserve(static_cast<size_t>(numSegments));
    const float step = kTwoPi / static_cast<float>(numSegments);
    for (int i = 0; i < numSegments; ++i)
    {
      const float angle = step * static_cast<float>(i);
      points.emplace_back(center + vec2(std::cos(angle), std::sin(angle)) * radius);
    }
    return addConvexFill(list, points.data(), points.size(), color, clipRect, SamplerModeToIndex(SamplerMode::Linear),
                         layer);
  }

  bool AddImage(Resource::ResourceManager& resources, PrimitiveDrawList& list, TextureHandle texture, const vec2& min,
                const vec2& max, const vec2& uvMin, const vec2& uvMax, uint32_t color, const vec4& clipRect,
                SamplerMode samplerMode, uint16_t layer)
  {
    if (!texture.isValid()) return false;
    const uint32_t bindless = resources.getTextureBindlessIndex(texture);
    const int32_t baseVertex = static_cast<int32_t>(list.vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(list.indices.size());
    const uint32_t samplerIndex = SamplerModeToIndex(samplerMode);
    PrimitiveDrawCommand& cmd = AcquireOrMergeCommand(list, bindless, samplerIndex, clipRect, layer, baseIndex,
                                                      baseVertex);
    list.vertices.reserve(list.vertices.size() + 4);
    list.vertices.push_back({min.x, min.y, uvMin.x, uvMin.y, color});
    list.vertices.push_back({max.x, min.y, uvMax.x, uvMin.y, color});
    list.vertices.push_back({max.x, max.y, uvMax.x, uvMax.y, color});
    list.vertices.push_back({min.x, max.y, uvMin.x, uvMax.y, color});
    const uint32_t base = static_cast<uint32_t>(baseVertex);
    list.indices.reserve(list.indices.size() + 6);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 1);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 3);
    cmd.indexCount = static_cast<uint32_t>(list.indices.size()) - cmd.indexOffset;
    return true;
  }

  bool AddImage(Resource::ResourceManager& resources, PrimitiveDrawList& list, uint32_t texture, const vec2& min,
                const vec2& max, const vec2& uvMin, const vec2& uvMax, uint32_t color, const vec4& clipRect,
                SamplerMode samplerMode, uint16_t layer)
  {
    const uint32_t bindless = texture;
    const int32_t baseVertex = static_cast<int32_t>(list.vertices.size());
    const uint32_t baseIndex = static_cast<uint32_t>(list.indices.size());
    const uint32_t samplerIndex = SamplerModeToIndex(samplerMode);
    PrimitiveDrawCommand& cmd = AcquireOrMergeCommand(list, bindless, samplerIndex, clipRect, layer, baseIndex,
                                                      baseVertex);
    list.vertices.reserve(list.vertices.size() + 4);
    list.vertices.push_back({min.x, min.y, uvMin.x, uvMin.y, color});
    list.vertices.push_back({max.x, min.y, uvMax.x, uvMin.y, color});
    list.vertices.push_back({max.x, max.y, uvMax.x, uvMax.y, color});
    list.vertices.push_back({min.x, max.y, uvMin.x, uvMax.y, color});
    const uint32_t base = static_cast<uint32_t>(baseVertex);
    list.indices.reserve(list.indices.size() + 6);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 1);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 0);
    list.indices.push_back(base + 2);
    list.indices.push_back(base + 3);
    cmd.indexCount = static_cast<uint32_t>(list.indices.size()) - cmd.indexOffset;
    return true;
  }

  bool AddText(Resource::ResourceManager& resources, PrimitiveDrawList& list, FontHandle font, std::string_view text,
               const TextLayoutDesc& layout, uint32_t color, uint16_t layer)
  {
    if (!font.isValid() || text.empty()) return false;
    const auto* fontHot = resources.getFont(font);
    if (!fontHot) return false;
    const uint32_t baseIndex = static_cast<uint32_t>(list.indices.size());
    const int32_t baseVertex = static_cast<int32_t>(list.vertices.size());
    const size_t estimatedGlyphs = text.size();
    list.vertices.reserve(list.vertices.size() + estimatedGlyphs * 4);
    list.indices.reserve(list.indices.size() + estimatedGlyphs * 6);
    const Resource::FontCPUData& fontCpu = fontHot->cpuData;
    bool emittedGlyph = IterateGlyphPlacements(fontCpu, text, layout, [&](const GlyphPlacement& placement)
    {
      const uint32_t base = static_cast<uint32_t>(list.vertices.size());
      list.vertices.push_back({placement.x0, placement.y0, placement.glyph->uvMin.x, placement.glyph->uvMin.y, color});
      list.vertices.push_back({placement.x1, placement.y0, placement.glyph->uvMax.x, placement.glyph->uvMin.y, color});
      list.vertices.push_back({placement.x1, placement.y1, placement.glyph->uvMax.x, placement.glyph->uvMax.y, color});
      list.vertices.push_back({placement.x0, placement.y1, placement.glyph->uvMin.x, placement.glyph->uvMax.y, color});
      list.indices.push_back(base + 0);
      list.indices.push_back(base + 1);
      list.indices.push_back(base + 2);
      list.indices.push_back(base + 0);
      list.indices.push_back(base + 2);
      list.indices.push_back(base + 3);
      return true;
    });
    if (!emittedGlyph) return false;
    PrimitiveDrawCommand& cmd = AcquireOrMergeCommand(list, fontHot->bindlessIndex,
                                                      SamplerModeToIndex(SamplerMode::Font), layout.clipRect, layer,
                                                      baseIndex, baseVertex);
    cmd.indexCount = static_cast<uint32_t>(list.indices.size()) - cmd.indexOffset;
    return true;
  }

  vec2 MeasureText(Resource::ResourceManager& resources, FontHandle font, std::string_view text,
                   const TextLayoutDesc& layout)
  {
    if (!font.isValid() || text.empty()) return vec2(0.0f);
    const auto* fontHot = resources.getFont(font);
    if (!fontHot) return vec2(0.0f);
    vec2 boundsMin{std::numeric_limits<float>::max()};
    vec2 boundsMax{std::numeric_limits<float>::lowest()};
    const Resource::FontCPUData& fontCpu = fontHot->cpuData;
    bool emittedGlyph = IterateGlyphPlacements(fontCpu, text, layout, [&](const GlyphPlacement& placement)
    {
      boundsMin.x = std::min(boundsMin.x, placement.x0);
      boundsMin.y = std::min(boundsMin.y, placement.y0);
      boundsMax.x = std::max(boundsMax.x, placement.x1);
      boundsMax.y = std::max(boundsMax.y, placement.y1);
      return true;
    });
    if (!emittedGlyph) return vec2(0.0f);
    return vec2(boundsMax.x - boundsMin.x, boundsMax.y - boundsMin.y);
  }
} // namespace ui
