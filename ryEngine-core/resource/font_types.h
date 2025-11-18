#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "math/utils_math.h"

namespace Resource
{
  struct FontGlyph
  {
    uint32_t codepoint = 0;
    vec2 uvMin{ 0.0f };
    vec2 uvMax{ 0.0f };
    vec2 sizePx{ 0.0f };
    vec2 bearingPx{ 0.0f };
    float advancePx = 0.0f;
  };

  struct FontBuildSettings
  {
    float pixelHeight = 32.0f;
    uint32_t firstCodepoint = 32;
    uint32_t lastCodepoint = 255;
    uint32_t fallbackCodepoint = '?';
    int oversample = 4;
    bool snapToPixel = false;
    float rasterizerMultiply = 1.0f;
    std::vector<uint32_t> extraCodepoints;
    bool enableKerning = true;
    float glyphMinAdvanceX = 0.0f;
  };

  struct KerningPair
  {
    uint64_t key = 0;
    float value = 0.0f;
  };

  struct GlyphLookupEntry
  {
    uint32_t codepoint = 0;
    uint32_t index = 0;
  };

  struct FontCPUData
  {
    std::string name;
    FontBuildSettings buildSettings;
    float ascent = 0.0f;
    float descent = 0.0f;
    float lineGap = 0.0f;
    uint32_t atlasWidth = 0;
    uint32_t atlasHeight = 0;
    vec2 whitePixelUV{ 0.0f };
    std::vector<uint8_t> atlasPixelsRGBA;
    std::vector<FontGlyph> glyphs;
    std::vector<GlyphLookupEntry> extraGlyphLookup;
    uint32_t baseGlyphCount = 0;
    std::vector<KerningPair> kerningPairs;
    bool kerningEnabled = true;

    const FontGlyph* findGlyph(uint32_t codepoint) const
    {
      const uint32_t first = buildSettings.firstCodepoint;
      if(codepoint >= first)
      {
        const uint32_t index = codepoint - first;
        if(index < baseGlyphCount && index < glyphs.size())
          return &glyphs[index];
      }
      const auto it = std::lower_bound(extraGlyphLookup.begin(),
                                       extraGlyphLookup.end(),
                                       codepoint,
                                       [](const GlyphLookupEntry& entry, uint32_t value)
                                       {
                                         return entry.codepoint < value;
                                       });
      if(it == extraGlyphLookup.end() || it->codepoint != codepoint)
        return nullptr;
      return &glyphs[it->index];
    }

    float getKerning(uint32_t left, uint32_t right) const
    {
      if(!kerningEnabled)
        return 0.0f;
      const uint64_t key = (static_cast<uint64_t>(left) << 32u) | static_cast<uint64_t>(right);
      const auto it = std::lower_bound(kerningPairs.begin(),
                                       kerningPairs.end(),
                                       key,
                                       [](const KerningPair& pair, uint64_t value)
                                       {
                                         return pair.key < value;
                                       });
      if(it == kerningPairs.end() || it->key != key)
        return 0.0f;
      return it->value;
    }
  };

  struct FontMergeSource
  {
    std::string name;
    std::vector<uint8_t> fontFileData;
    FontBuildSettings buildSettings;
    std::filesystem::path sourceFile;

    bool isValid() const
    {
      return !name.empty() && !fontFileData.empty();
    }
  };
} // namespace Resource
