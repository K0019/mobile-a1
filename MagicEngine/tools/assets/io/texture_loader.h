#pragma once
#include <filesystem>
#include <vector>
#include <resource/processed_assets.h>
#include "material_loader.h"
#include "resource/resource_types.h"
#include "import_config.h"
#include "renderer/gfx_interface.h"

struct aiScene;
struct aiTexture;

namespace Resource
{

  namespace TextureLoading
  {
    // Single texture extraction - fully parallelizable
    // isSRGB: true for color textures (baseColor, emissive), false for data textures (normal, metallicRoughness, occlusion)
    ProcessedTexture extractTexture(const TextureDataSource& source, const LoadingConfig& config, bool isSRGB = true);

    // Collection utilities - returns pairs of (source, isSRGB)
    std::vector<std::pair<TextureDataSource, bool>> collectUniqueTextures(const std::vector<ProcessedMaterial>& materials);

    // Utility functions
    bool isValidTexture(const ProcessedTexture& texture);

    std::string generateTextureCacheKey(const ProcessedTexture& texture);

    bool loadFromFile(const std::string& path, ProcessedTexture& texture, bool sRGB, gfx::TextureType type = gfx::TextureType::Tex2D);
    // Core loading implementations (internal use)
    namespace Detail
    {

      bool loadFromFileKTX(const std::filesystem::path& path, ProcessedTexture& texture, gfx::TextureType type);
      bool loadFromMemoryKTX(const uint8_t* data, size_t size, const std::string& path, ProcessedTexture& texture, gfx::TextureType type);
      bool loadFromFileKTX2(const std::filesystem::path& path, ProcessedTexture& texture, gfx::TextureType type);
      bool loadFromMemoryKTX2(const uint8_t* data, size_t size, const std::string& path, ProcessedTexture& texture, gfx::TextureType type);

      bool loadFromFileStandard(const std::filesystem::path& path, ProcessedTexture& texture, bool sRGB);

      bool loadFromMemory(const uint8_t* data, size_t size, ProcessedTexture& texture, const std::string& name, bool sRGB);

      bool loadFromEmbedded(const EmbeddedMemorySource& src, ProcessedTexture& texture, bool isSRGB);

      bool loadFromUncompressedEmbedded(const aiTexture* aiTex, ProcessedTexture& texture, bool isSRGB);
    } // namespace Detail
  }   // namespace TextureLoading
} // namespace AssetLoading
