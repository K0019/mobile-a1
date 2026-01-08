#pragma once
#include <span>
#include <string_view>
#include "resource/font_types.h"

namespace ui
{
  class FontSystem
  {
  public:
    static Resource::FontCPUData BuildFontCPUData(std::span<const uint8_t> fontData,
                                                  const Resource::FontBuildSettings& settings,
                                                  std::span<const Resource::FontMergeSource> mergeSources,
                                                  std::string_view debugName);
  };
} // namespace ui
