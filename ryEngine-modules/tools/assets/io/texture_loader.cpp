#include "VFS/VFS.h"
#include "texture_loader.h"
#include "logging/log.h"
#include "utilities/util.h"
#include <FreeImage.h>
#include <graphics/vulkan/vk_util.h>
#include <ktxvulkan.h>
#include <assimp/scene.h>
#include <algorithm>

namespace Resource::TextureLoading
{
  ProcessedTexture extractTexture(const TextureDataSource& source, [[maybe_unused]] const LoadingConfig& config)
  {
    ProcessedTexture texture;
    texture.source = source;

    // Generate name from source
    texture.name = std::visit([](const auto& src) -> std::string
    {
      using T = std::decay_t<decltype(src)>;
      if constexpr(std::is_same_v<T, std::monostate>) {
        return "null_texture";
      }
      else if constexpr(std::is_same_v<T, FilePathSource>)
      {
        return std::filesystem::path(src.path).filename().string();
      }
      else if constexpr(std::is_same_v<T, EmbeddedMemorySource>)
      {
        return "embedded_" + src.identifier;
      }
    },
                              source);

    try
    {
      // Load texture data based on source type
      bool loadResult = std::visit([&](const auto& src) -> bool
      {
        using T = std::decay_t<decltype(src)>;
        if constexpr(std::is_same_v<T, std::monostate>) {
          return false;
        }
        else if constexpr(std::is_same_v<T, FilePathSource>)
        {
          return loadFromFile(src.path, texture, true);
        }
        else if constexpr(std::is_same_v<T, EmbeddedMemorySource>)
        {
          return Detail::loadFromEmbedded(src, texture);
        }
      },
                                   source);

      if(!loadResult || texture.data.empty())
      {
        return texture;
      }
    }
    catch([[maybe_unused]] const std::exception& e)
    {
      texture.data.clear();
    }

    return texture;
  }

  std::vector<TextureDataSource> collectUniqueTextures(const std::vector<ProcessedMaterial>& materials)
  {
    std::unordered_set<std::string> seen;
    std::vector<TextureDataSource> uniqueTextures;

    for(const auto& material : materials)
    {
      auto addIfUnique = [&](const MaterialTexture& matTex)
      {
        if(!matTex.hasTexture())
          return;

        const std::string key = TextureCacheKeyGenerator::generateKey(matTex.source);
        if(!key.empty() && !seen.contains(key))
        {
          seen.insert(key);
          uniqueTextures.push_back(matTex.source);
        }
      };

      addIfUnique(material.baseColorTexture);
      addIfUnique(material.metallicRoughnessTexture);
      addIfUnique(material.normalTexture);
      addIfUnique(material.emissiveTexture);
      addIfUnique(material.occlusionTexture);
    }

    return uniqueTextures;
  }

  std::string generateTextureKey(const TextureDataSource& source)
  {
    return TextureCacheKeyGenerator::generateKey(source);
  }

  bool isValidTexture(const ProcessedTexture& texture)
  {
    return !texture.data.empty() && texture.width > 0 && texture.height > 0 && texture.channels > 0 && texture.textureDesc.format != vk::Format::Invalid;
  }
  bool loadFromFile(const std::string& path, ProcessedTexture& texture, bool sRGB, vk::TextureType type)
  {
      //if(!exists(path))
      //{
      //  LOG_WARNING("Texture file not found: {}", path.string());
      //  return false;
      //}
      //const std::string ext = path.extension().string();
      //if (ext == ".ktx" || ext == ".KTX")
      //{
      //    ASSERT(type == vk::TextureType::Tex2D);
      //    return Detail::loadFromFileKTX(path, texture, type);
      //}
      //if (ext == ".ktx2" || ext == ".KTX2")
      //{
      //    return Detail::loadFromFileKTX2(path, texture, type);
      //}
      //ASSERT(type == vk::TextureType::Tex2D);
      //return Detail::loadFromFileStandard(path, texture, sRGB);

      if (!VFS::FileExists(path))
      {
          LOG_WARNING("Texture file not found: {}", path);
          return false;
      }
      const std::string ext = VFS::GetExtension(path);

      std::vector<uint8_t> fileData;
      if (!VFS::ReadFile(path, fileData))
      {
          LOG_WARNING("VFS: Read texture file failed: {}", path);
          return false;
      }

      if (ext == ".ktx" || ext == ".KTX")
      {
          ASSERT(type == vk::TextureType::Tex2D);
          return Detail::loadFromMemoryKTX(fileData.data(), fileData.size(), path, texture, type);
      }
      if (ext == ".ktx2" || ext == ".KTX2")
      {
          return Detail::loadFromMemoryKTX2(fileData.data(), fileData.size(), path, texture, type);
      }

      ASSERT(type == vk::TextureType::Tex2D);
      return Detail::loadFromMemory(fileData.data(), fileData.size(), texture, path, sRGB);
  }
  namespace Detail
  {
    bool loadFromFileKTX(const std::filesystem::path& path, ProcessedTexture& texture, vk::TextureType
                         type)
    {
      ktxTexture1* ktxTex = nullptr;

      const auto result = ktxTexture1_CreateFromNamedFile(path.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

      if(result != KTX_SUCCESS || !ktxTex)
      {
        LOG_WARNING("Failed to load KTX file '{}'", path.string());
        return false;
      }

      SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

      // Extract texture properties
      texture.originalFileSize = file_size(path);
      texture.width = ktxTex->baseWidth;
      texture.height = ktxTex->baseHeight;
      texture.channels = 4; // Most KTX textures are 4-channel

      // Copy texture data
      const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
      texture.data.resize(dataSize);
      std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

      // Create descriptor
      texture.textureDesc = vk::TextureDesc{ .type = type, .format = vk::vkFormatToFormat(ktxTexture1_GetVkFormat(ktxTex)), .dimensions = {texture.width, texture.height, 1}, .usage = vk::TextureUsageBits_Sampled, .numMipLevels = ktxTex->numLevels, .debugName = path.string().c_str() };

      LOG_DEBUG("Loaded KTX texture '{}' - {}x{}, {} mips", path.string(), texture.width, texture.height, ktxTex->numLevels);

      return true;
    }

    bool loadFromMemoryKTX(const uint8_t* data, size_t size, const std::string& path, ProcessedTexture& texture, vk::TextureType type)
    {
        ktxTexture1* ktxTex = nullptr;

        const auto result = ktxTexture1_CreateFromMemory(data, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

        if (result != KTX_SUCCESS || !ktxTex)
        {
            LOG_WARNING("Failed to load KTX file '{}'", path);
            return false;
        }

        SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

        // Extract texture properties
        texture.originalFileSize = size;
        texture.width = ktxTex->baseWidth;
        texture.height = ktxTex->baseHeight;
        texture.channels = 4; // Most KTX textures are 4-channel

        // Copy texture data
        const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
        texture.data.resize(dataSize);
        std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

        // Create descriptor
        texture.textureDesc = vk::TextureDesc{ 
            .type = type, 
            .format = vk::vkFormatToFormat(ktxTexture1_GetVkFormat(ktxTex)), 
            .dimensions = {texture.width, texture.height, 1}, 
            .usage = vk::TextureUsageBits_Sampled, 
            .numMipLevels = ktxTex->numLevels, 
            .debugName = path.c_str() };

        LOG_DEBUG("Loaded KTX texture '{}' - {}x{}, {} mips", path, texture.width, texture.height, ktxTex->numLevels);

        return true;
    }

    bool loadFromFileKTX2(const std::filesystem::path& path, ProcessedTexture& texture, vk::TextureType type)
    {
        ktxTexture2* ktxTex = nullptr;

        KTX_error_code result = ktxTexture2_CreateFromNamedFile(path.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

        if (result != KTX_SUCCESS || !ktxTex)
        {
            LOG_WARNING("Failed to load KTX2 file '{}'. KTX Error: {}", path.string(), ktxErrorString(result));
            if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
            return false;
        }

        if (ktxTex->classId != class_id::ktxTexture2_c)
        {
            LOG_WARNING("File '{}' is not a KTX2 file. Found KTX1 instead.", path.string());
            ktxTexture_Destroy(ktxTexture(ktxTex));
            return false;
        }

        SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

        // Extract texture properties
        texture.originalFileSize = std::filesystem::file_size(path);
        texture.width = ktxTex->baseWidth;
        texture.height = ktxTex->baseHeight;
        texture.channels = 4; // Most KTX textures are 4-channel

        // Copy texture data
        const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
        texture.data.resize(dataSize);
        std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

        // Create descriptor
        texture.textureDesc = vk::TextureDesc{
            .type = type,
            .format = vk::vkFormatToFormat(static_cast<VkFormat>(ktxTex->vkFormat)),
            .dimensions = {texture.width, texture.height, 1},
            .usage = vk::TextureUsageBits_Sampled,
            .numMipLevels = ktxTex->numLevels,
            .debugName = path.string().c_str() };

        LOG_DEBUG("Loaded KTX2 texture '{}' - {}x{}, {} mips",
            path.string(), texture.width, texture.height, ktxTex->numLevels/*, vk::getFormatName(texture.textureDesc.format)*/);

        return true;
    }

    bool loadFromMemoryKTX2(const uint8_t* data, size_t size, const std::string& path, ProcessedTexture& texture, vk::TextureType type)
    {
        ktxTexture2* ktxTex = nullptr;

        KTX_error_code result = ktxTexture2_CreateFromMemory(data, size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTex);

        if (result != KTX_SUCCESS || !ktxTex)
        {
            LOG_WARNING("Failed to load KTX2 file '{}'. KTX Error: {}", path, ktxErrorString(result));
            if (ktxTex) ktxTexture_Destroy(ktxTexture(ktxTex));
            return false;
        }

        if (ktxTex->classId != class_id::ktxTexture2_c)
        {
            LOG_WARNING("File '{}' is not a KTX2 file. Found KTX1 instead.", path);
            ktxTexture_Destroy(ktxTexture(ktxTex));
            return false;
        }

        SCOPE_EXIT{ ktxTexture_Destroy(ktxTexture(ktxTex)); };

        // Extract texture properties
        texture.originalFileSize = size;
        texture.width = ktxTex->baseWidth;
        texture.height = ktxTex->baseHeight;
        texture.channels = 4; // Most KTX textures are 4-channel

        // Copy texture data
        const size_t dataSize = ktxTexture_GetDataSize(ktxTexture(ktxTex));
        texture.data.resize(dataSize);
        std::memcpy(texture.data.data(), ktxTex->pData, dataSize);

        // Create descriptor
        texture.textureDesc = vk::TextureDesc{
            .type = type,
            .format = vk::vkFormatToFormat(static_cast<VkFormat>(ktxTex->vkFormat)),
            .dimensions = {texture.width, texture.height, 1},
            .usage = vk::TextureUsageBits_Sampled,
            .numMipLevels = ktxTex->numLevels,
            .debugName = path.c_str() };

        LOG_DEBUG("Loaded KTX2 texture '{}' - {}x{}, {} mips",
            path, texture.width, texture.height, ktxTex->numLevels/*, vk::getFormatName(texture.textureDesc.format)*/);

        return true;
    }

    bool loadFromFileStandard(const std::filesystem::path& path, ProcessedTexture& texture, bool sRGB)
    {
      // Detect file format
        std::string test{ path.string() };
        std::replace(test.begin(), test.end(), '\\', '/');
      FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(test.c_str(), 0);
      if(fif == FIF_UNKNOWN) {
        fif = FreeImage_GetFIFFromFilename(test.c_str());
      }

      if(fif == FIF_UNKNOWN || !FreeImage_FIFSupportsReading(fif))
      {
        LOG_WARNING("Unsupported image format '{}'", path.string());
        return false;
      }

      // Load image
      FIBITMAP* dib = FreeImage_Load(fif, path.string().c_str(), 0);
      if(!dib)
      {
        LOG_WARNING("FreeImage Load failed for '{}'", path.string());
        return false;
      }
      SCOPE_EXIT{ FreeImage_Unload(dib); };

      // Convert to 32-bit RGBA
      FreeImage_FlipVertical(dib); // Correct orientation
      FIBITMAP* dib32 = FreeImage_ConvertTo32Bits(dib);
      if(!dib32)
      {
        LOG_WARNING("Failed to convert to 32-bit for '{}'", path.string());
        return false;
      }
      SCOPE_EXIT{ FreeImage_Unload(dib32); };

      // Extract image data
      const unsigned int width = FreeImage_GetWidth(dib32);
      const unsigned int height = FreeImage_GetHeight(dib32);
      const unsigned char* imgData = FreeImage_GetBits(dib32);

      if(!imgData || width == 0 || height == 0)
      {
        LOG_WARNING("Invalid image data in '{}'", path.string());
        return false;
      }

      // Populate texture
      texture.originalFileSize = std::filesystem::file_size(path);
      texture.width = width;
      texture.height = height;
      texture.channels = 4;
      texture.sRGB = sRGB;

      // Copy image data
      const size_t dataSize = width * height * 4;
      texture.data.resize(dataSize);
      std::memcpy(texture.data.data(), imgData, dataSize);

      // Create descriptor with correct format for FreeImage's BGRA order
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      const vk::Format format = sRGB ? vk::Format::BGRA_SRGB8 : vk::Format::BGRA_UN8;
#else
      const vk::Format format = sRGB ? vk::Format::RGBA_SRGB8 : vk::Format::RGBA_UN8;
#endif

      texture.textureDesc = vk::TextureDesc{ .type = vk::TextureType::Tex2D, .format = format, .dimensions = {texture.width, texture.height, 1}, .usage = vk::TextureUsageBits_Sampled, .numMipLevels = 1, .debugName = path.string().c_str() };

      LOG_DEBUG("Loaded texture '{}' - {}x{}", path.string(), width, height);
      return true;
    }

    bool loadFromEmbedded(const EmbeddedMemorySource& src, ProcessedTexture& texture)
    {
      if(!src.scene || src.identifier.empty() || src.identifier[0] != '*') {
        return false;
      }

      size_t embeddedIndex;
      try {
        embeddedIndex = std::stoul(src.identifier.substr(1));
      }
      catch(const std::exception&)
      {
        LOG_WARNING("Invalid embedded texture identifier: {}", src.identifier);
        return false;
      }

      if(embeddedIndex >= src.scene->mNumTextures)
      {
        LOG_WARNING("Embedded texture index {} out of range", embeddedIndex);
        return false;
      }

      const aiTexture* aiTex = src.scene->mTextures[embeddedIndex];
      if(!aiTex)
        return false;

      if(aiTex->mHeight == 0)
      {
        // Compressed embedded texture
        const uint8_t* data = reinterpret_cast<const uint8_t*>(aiTex->pcData);
        return loadFromMemory(data, aiTex->mWidth, texture, texture.name, true);
      }
      else
      {
        // Uncompressed RGBA data
        return loadFromUncompressedEmbedded(aiTex, texture);
      }
    }

    bool loadFromUncompressedEmbedded(const aiTexture* aiTex, ProcessedTexture& texture)
    {
      const size_t dataSize = aiTex->mWidth * aiTex->mHeight * 4;
      const uint8_t* data = reinterpret_cast<const uint8_t*>(aiTex->pcData);

      texture.width = aiTex->mWidth;
      texture.height = aiTex->mHeight;
      texture.channels = 4;
      texture.data.assign(data, data + dataSize);
      texture.sRGB = true;
      texture.originalFileSize = dataSize;

      texture.textureDesc = vk::TextureDesc{ .type = vk::TextureType::Tex2D, .format = vk::Format::RGBA_UN8, .dimensions = {texture.width, texture.height, 1}, .usage = vk::TextureUsageBits_Sampled, .numMipLevels = 1, .debugName = texture.name.c_str() };

      return true;
    }

    bool loadFromMemory(const uint8_t* data, size_t size, ProcessedTexture& texture, const std::string& name, bool sRGB)
    {
      if(!data || size == 0)
      {
        LOG_WARNING("Invalid memory data for texture: {}", name);
        return false;
      }

      FIMEMORY* stream = FreeImage_OpenMemory(const_cast<unsigned char*>(data), static_cast<DWORD>(size));
      if(!stream)
      {
        LOG_WARNING("FreeImage OpenMemory failed for texture: {}", name);
        return false;
      }
      SCOPE_EXIT{ FreeImage_CloseMemory(stream); };

      FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(stream, 0);
      if(fif == FIF_UNKNOWN)
      {
        LOG_WARNING("Unknown image format for texture: {}", name);
        return false;
      }

      FIBITMAP* dib = FreeImage_LoadFromMemory(fif, stream, 0);
      if(!dib)
      {
        LOG_WARNING("Failed to load texture from memory: {}", name);
        return false;
      }
      SCOPE_EXIT{ FreeImage_Unload(dib); };

      // Same processing as file-based loading
      FreeImage_FlipVertical(dib);
      FIBITMAP* dib32 = FreeImage_ConvertTo32Bits(dib);
      if(!dib32)
      {
        LOG_WARNING("Failed to convert embedded texture to 32-bit: {}", name);
        return false;
      }
      SCOPE_EXIT{ FreeImage_Unload(dib32); };

      const unsigned int width = FreeImage_GetWidth(dib32);
      const unsigned int height = FreeImage_GetHeight(dib32);
      const unsigned char* imgData = FreeImage_GetBits(dib32);

      if(!imgData || width == 0 || height == 0)
      {
        LOG_WARNING("Invalid embedded texture data: {}", name);
        return false;
      }

      texture.originalFileSize = size;
      texture.width = width;
      texture.height = height;
      texture.channels = 4;
      texture.sRGB = sRGB;

      const size_t dataSize = width * height * 4;
      texture.data.resize(dataSize);
      std::memcpy(texture.data.data(), imgData, dataSize);

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
      const vk::Format format = sRGB ? vk::Format::BGRA_SRGB8 : vk::Format::BGRA_UN8;
#else
      const vk::Format format = sRGB ? vk::Format::RGBA_SRGB8 : vk::Format::RGBA_UN8;
#endif

      texture.textureDesc = vk::TextureDesc{ .type = vk::TextureType::Tex2D, .format = format, .dimensions = {texture.width, texture.height, 1}, .usage = vk::TextureUsageBits_Sampled, .numMipLevels = 1, .debugName = name.c_str() };

      LOG_DEBUG("Loaded embedded texture '{}' - {}x{}", name, width, height);
      return true;
    }
  } // namespace Detail
}   // namespace AssetLoading::TextureLoading

