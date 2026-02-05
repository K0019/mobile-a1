#pragma once
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>
#include "math/utils_math.h"
#include "renderer/gfx_interface.h"
#include "resource/resource_types.h"

namespace Resource
{
  class ResourceManager;
}

namespace ui
{
  enum class SamplerMode : uint8_t
  {
    Linear = 0,
    Nearest = 1,
    Font = 2
  };

  inline uint32_t SamplerModeToIndex(SamplerMode mode)
  {
    return static_cast<uint32_t>(mode);
  }

  struct PrimitiveVertex
  {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    uint32_t color = 0xFFFFFFFFu;
  };

  struct PrimitiveDrawCommand
  {
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    int32_t vertexOffset = 0;
    uint32_t sortOrder = 0;
    uint16_t layer = 0;
    vec4 clipRect{0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    uint64_t textureId = 0;
    uint32_t samplerIndex = 0;
    // Optional: direct texture view (bypasses textureId resolution when valid)
    // Used for runtime textures like video frames that aren't in the asset system
    gfx::TextureView directView = {};
  };

  struct PrimitiveDrawList
  {
    std::vector<PrimitiveVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<PrimitiveDrawCommand> commands;
    uint64_t solidFillTextureId = 0;
    vec2 whitePixelUV{0.0f};
    uint32_t commandOrderCounter = 0;

    void clear()
    {
      vertices.clear();
      indices.clear();
      commands.clear();
      commandOrderCounter = 0;
    }

    bool empty() const
    {
      return commands.empty();
    }

    void setSolidFillFallback(uint64_t textureId, const vec2& uv)
    {
      solidFillTextureId = textureId;
      whitePixelUV = uv;
    }

    bool hasSolidFillFallback() const
    {
      return solidFillTextureId != 0;
    }
  };

  struct TextLayoutDesc
  {
    vec2 origin{0.0f};
    vec4 clipRect{0.0f, 0.0f, std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
    float lineSpacing = 1.0f;
    float letterSpacing = 0.0f;
    float wrapWidth = 0.0f;
    bool snapToPixel = true;
    bool useFontSnapSetting = true;
    bool originIsBaseline = false;
  };

  bool ApplySolidFillFallback(PrimitiveDrawList& list, Resource::ResourceManager& resources, FontHandle font);

  uint32_t PackColor(const vec4& color);

  void AddSolidRect(PrimitiveDrawList& list, const vec2& min, const vec2& max, uint32_t color, const vec4& clipRect,
                    uint16_t layer = 0);

  bool AddLine(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, uint32_t color, float thickness,
               const vec4& clipRect, uint16_t layer = 0);

  bool AddPolyline(PrimitiveDrawList& list, const vec2* points, size_t pointCount, bool closed, uint32_t color,
                   float thickness, const vec4& clipRect, uint16_t layer = 0);

  void AddRectOutline(PrimitiveDrawList& list, const vec2& min, const vec2& max, float thickness, uint32_t color,
                      const vec4& clipRect, uint16_t layer = 0);

  bool AddTriangle(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, const vec2& p3, uint32_t color,
                   float thickness, const vec4& clipRect, uint16_t layer = 0);

  bool AddTriangleFilled(PrimitiveDrawList& list, const vec2& p1, const vec2& p2, const vec2& p3, uint32_t color,
                         const vec4& clipRect, uint16_t layer = 0);

  bool AddCircle(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                 float thickness, const vec4& clipRect, uint16_t layer = 0);

  bool AddCircleFilled(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                       const vec4& clipRect, uint16_t layer = 0);

  bool AddNgon(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
               float thickness, const vec4& clipRect, uint16_t layer = 0);

  bool AddNgonFilled(PrimitiveDrawList& list, const vec2& center, float radius, uint32_t color, int numSegments,
                     const vec4& clipRect, uint16_t layer = 0);

  bool AddImage(Resource::ResourceManager& resources, PrimitiveDrawList& list, TextureHandle texture, const vec2& min,
                const vec2& max, const vec2& uvMin = vec2(0.0f), const vec2& uvMax = vec2(1.0f),
                uint32_t color = 0xFFFFFFFFu,
                const vec4& clipRect = vec4(0.0f, 0.0f, std::numeric_limits<float>::max(),
                                            std::numeric_limits<float>::max()),
                SamplerMode samplerMode = SamplerMode::Linear, uint16_t layer = 0);

  bool AddImage(Resource::ResourceManager& resources, PrimitiveDrawList& list, uint64_t texture, const vec2& min,
                const vec2& max, const vec2& uvMin = vec2(0.0f), const vec2& uvMax = vec2(1.0f),
                uint32_t color = 0xFFFFFFFFu,
                const vec4& clipRect = vec4(0.0f, 0.0f, std::numeric_limits<float>::max(),
                                            std::numeric_limits<float>::max()),
                SamplerMode samplerMode = SamplerMode::Linear, uint16_t layer = 0);

  // Direct TextureView overload for runtime textures (video frames, procedural, etc.)
  // Bypasses texture ID resolution - the view is used directly in rendering
  bool AddImageDirect(PrimitiveDrawList& list, gfx::TextureView view, const vec2& min, const vec2& max,
                      const vec2& uvMin = vec2(0.0f), const vec2& uvMax = vec2(1.0f),
                      uint32_t color = 0xFFFFFFFFu,
                      const vec4& clipRect = vec4(0.0f, 0.0f, std::numeric_limits<float>::max(),
                                                  std::numeric_limits<float>::max()),
                      SamplerMode samplerMode = SamplerMode::Linear, uint16_t layer = 0);

  bool AddText(Resource::ResourceManager& resources, PrimitiveDrawList& list, FontHandle font, std::string_view text,
               const TextLayoutDesc& layout, uint32_t color, uint16_t layer = 0);

  vec2 MeasureText(Resource::ResourceManager& resources, FontHandle font, std::string_view text,
                   const TextLayoutDesc& layout);
} // namespace ui
