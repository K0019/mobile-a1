#include "renderer/ui/font_system.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "logging/log.h"
#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#define STBRP_STATIC
#define STBTT_STATIC
#include "imstb_rectpack.h"
#include "imstb_truetype.h"

namespace ui
{
  namespace
  {
    constexpr uint32_t kMaxAtlasDimension = 2048;

    void convertMonoToRGBA(const std::vector<uint8_t>& mono, uint32_t width, uint32_t height, float rasterizerMultiply,
                           std::vector<uint8_t>& out)
    {
      const size_t pixelCount = static_cast<size_t>(width) * height;
      out.resize(pixelCount * 4);
      const float multiplier = std::max(0.0f, rasterizerMultiply);
      for (size_t i = 0; i < pixelCount; ++i)
      {
        float scaled = static_cast<float>(mono[i]) * multiplier;
        if (multiplier == 0.0f)
        {
          scaled = 0.0f;
        }
        const uint32_t alpha = static_cast<uint32_t>(std::clamp(scaled + 0.5f, 0.0f, 255.0f));
        const uint32_t pixel = (alpha << 24u) | 0x00FFFFFFu;
        std::memcpy(out.data() + (i * 4), &pixel, sizeof(pixel));
      }
    }

    struct FontPackJob
    {
      std::span<const uint8_t> fontData;
      Resource::FontBuildSettings settings;
      std::string debugName;
      uint32_t baseGlyphCount = 0;
      std::vector<uint32_t> extraCodepoints;
      std::vector<int> extraCodepointsInt;
      std::vector<uint32_t> glyphCodepoints;
      std::vector<stbtt_packedchar> packedGlyphs;
      bool contributesToBase = false;
    };

    void sanitizeExtraCodepoints(const Resource::FontBuildSettings& settings, std::vector<uint32_t>& extras)
    {
      extras.erase(std::remove_if(extras.begin(), extras.end(), [&](uint32_t codepoint)
      {
        return codepoint >= settings.firstCodepoint && codepoint <= settings.lastCodepoint;
      }), extras.end());
      std::sort(extras.begin(), extras.end());
      extras.erase(std::unique(extras.begin(), extras.end()), extras.end());
    }

    FontPackJob buildPackJob(std::span<const uint8_t> data, const Resource::FontBuildSettings& settings,
                             std::string_view debugName, bool contributesToBase)
    {
      FontPackJob job;
      job.fontData = data;
      job.settings = settings;
      job.debugName = debugName.empty() ? "FontLayer" : std::string(debugName);
      job.contributesToBase = contributesToBase;
      if (settings.lastCodepoint >= settings.firstCodepoint)
      {
        job.baseGlyphCount = settings.lastCodepoint - settings.firstCodepoint + 1u;
      }
      job.extraCodepoints = settings.extraCodepoints;
      sanitizeExtraCodepoints(settings, job.extraCodepoints);
      job.extraCodepointsInt.assign(job.extraCodepoints.begin(), job.extraCodepoints.end());
      const size_t totalGlyphs = static_cast<size_t>(job.baseGlyphCount) + job.extraCodepoints.size();
      job.packedGlyphs.resize(totalGlyphs);
      job.glyphCodepoints.reserve(totalGlyphs);
      for (uint32_t i = 0; i < job.baseGlyphCount; ++i)
      {
        job.glyphCodepoints.push_back(settings.firstCodepoint + i);
      }
      job.glyphCodepoints.insert(job.glyphCodepoints.end(), job.extraCodepoints.begin(), job.extraCodepoints.end());
      return job;
    }
  } // namespace
  Resource::FontCPUData FontSystem::BuildFontCPUData(std::span<const uint8_t> fontData,
                                                     const Resource::FontBuildSettings& settings,
                                                     std::span<const Resource::FontMergeSource> mergeSources,
                                                     std::string_view debugName)
  {
    Resource::FontCPUData result;
    result.name = debugName.empty() ? "Font" : std::string(debugName);
    result.buildSettings = settings;
    if (fontData.empty())
    {
      LOG_ERROR("FontSystem: empty font data for '{}'", result.name);
      return result;
    }
    if (settings.lastCodepoint < settings.firstCodepoint)
    {
      LOG_WARNING("FontSystem: invalid codepoint range for '{}'", result.name);
      return result;
    }
    stbtt_fontinfo baseFontInfo;
    if (!stbtt_InitFont(&baseFontInfo, fontData.data(), 0))
    {
      LOG_ERROR("FontSystem: failed to initialize font '{}'", result.name);
      return result;
    }
    int ascent = 0, descent = 0, lineGap = 0;
    stbtt_GetFontVMetrics(&baseFontInfo, &ascent, &descent, &lineGap);
    const float scale = stbtt_ScaleForPixelHeight(&baseFontInfo, settings.pixelHeight);
    result.ascent = ascent * scale;
    result.descent = descent * scale;
    result.lineGap = lineGap * scale;
    std::vector<FontPackJob> jobs;
    jobs.reserve(1 + mergeSources.size());
    jobs.push_back(buildPackJob(fontData, settings, result.name, true));
    for (const auto& merge : mergeSources)
    {
      if (!merge.isValid())
      {
        LOG_WARNING("FontSystem: skipping invalid merge source for '{}'", result.name);
        continue;
      }
      if (merge.buildSettings.lastCodepoint < merge.buildSettings.firstCodepoint)
      {
        LOG_WARNING("FontSystem: invalid merge codepoint range for '{}' ({} - {})", merge.name.c_str(),
                    merge.buildSettings.firstCodepoint, merge.buildSettings.lastCodepoint);
        continue;
      }
      std::span<const uint8_t> mergeData(merge.fontFileData.data(), merge.fontFileData.size());
      jobs.push_back(buildPackJob(mergeData, merge.buildSettings, merge.name.empty() ? result.name : merge.name,
                                  false));
    }
    const size_t totalGlyphCount = std::accumulate(jobs.begin(), jobs.end(), size_t(0),
                                                   [](size_t sum, const FontPackJob& job)
                                                   {
                                                     return sum + job.glyphCodepoints.size();
                                                   });
    std::vector<uint8_t> atlasMono;
    uint32_t atlasDim = 256;
    bool packed = false;
    const int oversample = std::max(1, std::accumulate(jobs.begin(), jobs.end(), 1,
                                                       [](int current, const FontPackJob& job)
                                                       {
                                                         return std::max(current, job.settings.oversample);
                                                       }));
    while (atlasDim <= kMaxAtlasDimension)
    {
      atlasMono.assign(static_cast<size_t>(atlasDim) * atlasDim, 0);
      stbtt_pack_context packCtx;
      if (stbtt_PackBegin(&packCtx, atlasMono.data(), atlasDim, atlasDim, 0, 1, nullptr) == 0)
      {
        atlasDim <<= 1;
        continue;
      }
      stbtt_PackSetOversampling(&packCtx, oversample, oversample);
      bool allPacked = true;
      for (auto& job : jobs)
      {
        if (job.fontData.empty())
        {
          allPacked = false;
          break;
        }
        std::vector<stbtt_pack_range> ranges;
        if (job.baseGlyphCount > 0)
        {
          stbtt_pack_range baseRange{};
          baseRange.first_unicode_codepoint_in_range = job.settings.firstCodepoint;
          baseRange.num_chars = job.baseGlyphCount;
          baseRange.chardata_for_range = job.packedGlyphs.data();
          baseRange.font_size = job.settings.pixelHeight;
          ranges.push_back(baseRange);
        }
        if (!job.extraCodepointsInt.empty())
        {
          stbtt_pack_range extraRange{};
          extraRange.array_of_unicode_codepoints = job.extraCodepointsInt.data();
          extraRange.num_chars = static_cast<int>(job.extraCodepointsInt.size());
          extraRange.chardata_for_range = job.packedGlyphs.data() + job.baseGlyphCount;
          extraRange.font_size = job.settings.pixelHeight;
          ranges.push_back(extraRange);
        }
        if (ranges.empty())
        {
          continue;
        }
        if (stbtt_PackFontRanges(&packCtx, job.fontData.data(), 0, ranges.data(), static_cast<int>(ranges.size())) == 0)
        {
          allPacked = false;
          break;
        }
      }
      stbtt_PackEnd(&packCtx);
      if (allPacked)
      {
        packed = true;
        break;
      }
      atlasDim <<= 1;
    }
    if (!packed)
    {
      LOG_ERROR("FontSystem: failed to pack glyphs for '{}'", result.name);
      return result;
    }
    result.atlasWidth = atlasDim;
    result.atlasHeight = atlasDim;
    convertMonoToRGBA(atlasMono, atlasDim, atlasDim, settings.rasterizerMultiply, result.atlasPixelsRGBA);
    if (!result.atlasPixelsRGBA.empty())
    {
      result.atlasPixelsRGBA[0] = 255;
      result.atlasPixelsRGBA[1] = 255;
      result.atlasPixelsRGBA[2] = 255;
      result.atlasPixelsRGBA[3] = 255;
      result.whitePixelUV = vec2(0.5f / static_cast<float>(atlasDim), 0.5f / static_cast<float>(atlasDim));
    }
    result.glyphs.clear();
    result.glyphs.reserve(totalGlyphCount);
    std::vector<Resource::GlyphLookupEntry> extraLookup;
    uint32_t glyphIndex = 0;
    for (const auto& job : jobs)
    {
      for (size_t i = 0; i < job.glyphCodepoints.size(); ++i)
      {
        const stbtt_packedchar& glyphData = job.packedGlyphs[i];
        Resource::FontGlyph glyph;
        glyph.codepoint = job.glyphCodepoints[i];
        glyph.uvMin = vec2(glyphData.x0 / static_cast<float>(atlasDim), glyphData.y0 / static_cast<float>(atlasDim));
        glyph.uvMax = vec2(glyphData.x1 / static_cast<float>(atlasDim), glyphData.y1 / static_cast<float>(atlasDim));
        glyph.sizePx = vec2(static_cast<float>(glyphData.x1 - glyphData.x0) * 7,
                            static_cast<float>(glyphData.y1 - glyphData.y0) * 7 );
        glyph.bearingPx = vec2(glyphData.xoff * 7, glyphData.yoff * 7);
        glyph.advancePx = glyphData.xadvance * 7;
        if (job.settings.glyphMinAdvanceX > 0.0f)
        {
          glyph.advancePx = (std::max)(glyph.advancePx, job.settings.glyphMinAdvanceX);
        }
        result.glyphs.push_back(glyph);
        const bool outsideBaseRange = glyph.codepoint < settings.firstCodepoint || glyph.codepoint > settings.
          lastCodepoint;
        if (outsideBaseRange || glyphIndex >= jobs.front().baseGlyphCount)
        {
          extraLookup.push_back({glyph.codepoint, glyphIndex});
        }
        ++glyphIndex;
      }
    }
    result.baseGlyphCount = jobs.empty() ? 0u : jobs.front().baseGlyphCount;
    std::sort(extraLookup.begin(), extraLookup.end(),
              [](const Resource::GlyphLookupEntry& a, const Resource::GlyphLookupEntry& b)
              {
                return a.codepoint < b.codepoint;
              });
    extraLookup.erase(std::unique(extraLookup.begin(), extraLookup.end(),
                                  [](const Resource::GlyphLookupEntry& a, const Resource::GlyphLookupEntry& b)
                                  {
                                    return a.codepoint == b.codepoint;
                                  }), extraLookup.end());
    result.extraGlyphLookup = std::move(extraLookup);
    if (settings.enableKerning)
    {
      const size_t glyphCount = result.glyphs.size();
      result.kerningPairs.reserve(glyphCount * 2);
      for (const auto& left : result.glyphs)
      {
        for (const auto& right : result.glyphs)
        {
          const int kern = stbtt_GetCodepointKernAdvance(&baseFontInfo, left.codepoint, right.codepoint);
          if (kern == 0) continue;
          const uint64_t key = (static_cast<uint64_t>(left.codepoint) << 32u) | static_cast<uint64_t>(right.codepoint);
          result.kerningPairs.push_back({key, static_cast<float>(kern) * scale});
        }
      }
      if (!result.kerningPairs.empty())
      {
        std::sort(result.kerningPairs.begin(), result.kerningPairs.end(),
                  [](const Resource::KerningPair& a, const Resource::KerningPair& b)
                  {
                    return a.key < b.key;
                  });
        result.kerningPairs.erase(std::unique(result.kerningPairs.begin(), result.kerningPairs.end(),
                                              [](const Resource::KerningPair& a, const Resource::KerningPair& b)
                                              {
                                                return a.key == b.key;
                                              }), result.kerningPairs.end());
      }
      result.kerningEnabled = true;
    }
    else
    {
      result.kerningEnabled = false;
    }
    if (!result.glyphs.empty())
    {
      const uint32_t fallback = settings.fallbackCodepoint;
      const uint32_t first = settings.firstCodepoint;
      const uint32_t last = settings.lastCodepoint;
      const bool inBaseRange = fallback >= first && fallback <= last;
      bool hasFallback = inBaseRange;
      if (!hasFallback)
      {
        const auto it = std::lower_bound(result.extraGlyphLookup.begin(), result.extraGlyphLookup.end(), fallback,
                                         [](const Resource::GlyphLookupEntry& entry, uint32_t value)
                                         {
                                           return entry.codepoint < value;
                                         });
        hasFallback = (it != result.extraGlyphLookup.end() && it->codepoint == fallback);
      }
      if (!hasFallback)
      {
        const auto insertPos = std::lower_bound(result.extraGlyphLookup.begin(), result.extraGlyphLookup.end(),
                                                fallback, [](const Resource::GlyphLookupEntry& entry, uint32_t value)
                                                {
                                                  return entry.codepoint < value;
                                                });
        result.extraGlyphLookup.insert(insertPos, {fallback, 0u});
      }
    }
    return result;
  }
} // namespace ui
