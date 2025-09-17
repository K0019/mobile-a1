#pragma once
#include <filesystem>
#include <vector>
#include <unordered_set>
#include "material_loader.h"
#include "assets/core/asset_types.h"
#include "assets/core/import_config.h"
#include "graphics/interface.h"

struct aiScene;
struct aiTexture;

namespace AssetLoading
{
  struct ProcessedTexture
  {
    TextureDataSource source;
    std::string name;
    std::vector<uint8_t> data;
    vk::TextureDesc textureDesc;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    bool sRGB = true;
    size_t originalFileSize = 0;
  };

  namespace TextureLoading
  {
    // Single texture extraction - fully parallelizable
    ProcessedTexture extractTexture(const TextureDataSource& source, const LoadingConfig& config);

    // Collection utilities
    std::vector<TextureDataSource> collectUniqueTextures(const std::vector<ProcessedMaterial>& materials);

    // Utility functions
    bool isValidTexture(const ProcessedTexture& texture);

    std::string generateTextureCacheKey(const ProcessedTexture& texture);

    // Core loading implementations (internal use)
    namespace Detail
    {
      bool loadFromFile(const std::filesystem::path& path, ProcessedTexture& texture, bool sRGB);

      bool loadFromFileKTX(const std::filesystem::path& path, ProcessedTexture& texture);

      bool loadFromFileStandard(const std::filesystem::path& path, ProcessedTexture& texture, bool sRGB);

      bool loadFromMemory(const uint8_t* data, size_t size, ProcessedTexture& texture, const std::string& name, bool sRGB);

      bool loadFromEmbedded(const EmbeddedMemorySource& src, ProcessedTexture& texture);

      bool loadFromUncompressedEmbedded(const aiTexture* aiTex, ProcessedTexture& texture);
    }
  }

  class TextureCacheKeyGenerator
  {
    public:
      static std::string generateKey(const TextureDataSource& source);
  };
} // namespace AssetLoading
