/******************************************************************************/
/*!
\file   TextureCompiler.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Compresses a texture with compressonator.
The only output is .ktx2 right now.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TextureCompiler.h"
#include "resource/asset_metadata.h"  // For writing .meta sidecar files

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize2.h>

// ARM's official ASTC encoder (much more reliable than Compressonator's ASTC)
#include <astcenc.h>

#include <filesystem>
#include <cassert>
#include <thread>
#include <iostream>
#include <chrono>

namespace
{
    CMP_FORMAT MapCMPFormat(compiler::TextureChannelFormat fmt)
    {
        switch (fmt)
        {
        case compiler::TextureChannelFormat::RGBA_8888: return CMP_FORMAT_RGBA_8888;
        case compiler::TextureChannelFormat::ARGB_8888: return CMP_FORMAT_ARGB_8888;
        case compiler::TextureChannelFormat::RGBA_16F: return CMP_FORMAT_RGBA_16F;
        case compiler::TextureChannelFormat::ARGB_16F: return CMP_FORMAT_ARGB_16F;
        case compiler::TextureChannelFormat::ARGB_32F: return CMP_FORMAT_ARGB_32F;
        case compiler::TextureChannelFormat::RGBA_1010102: return CMP_FORMAT_RGBA_1010102;
        default: return CMP_FORMAT_RGBA_8888;
        }
    }

    CMP_FORMAT MapCMPFormat(compiler::TextureCompressionFormat fmt)
    {
        switch (fmt)
        {
        case compiler::TextureCompressionFormat::UNCOMPRESSED: return CMP_FORMAT_RGBA_8888;
        case compiler::TextureCompressionFormat::BC1: return CMP_FORMAT_BC1;
        case compiler::TextureCompressionFormat::BC3: return CMP_FORMAT_BC3;
        case compiler::TextureCompressionFormat::BC4: return CMP_FORMAT_BC4;
        case compiler::TextureCompressionFormat::BC5: return CMP_FORMAT_BC5;
        case compiler::TextureCompressionFormat::BC7: return CMP_FORMAT_BC7;
        case compiler::TextureCompressionFormat::ASTC: return CMP_FORMAT_ASTC;
        // Use ETC2 RGBA for full alpha support on Android (supported on all OpenGL ES 3.0+ devices)
        case compiler::TextureCompressionFormat::ETC: return CMP_FORMAT_ETC2_RGBA;
        default: return CMP_FORMAT_BC7;
        }
    }

    VkFormat MapVkFormat(compiler::TextureCompressionFormat fmt, bool isSRGB)
    {
        switch (fmt)
        {
        case compiler::TextureCompressionFormat::UNCOMPRESSED: return isSRGB ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;

        case compiler::TextureCompressionFormat::BC1: return isSRGB ? VK_FORMAT_BC1_RGB_SRGB_BLOCK : VK_FORMAT_BC1_RGB_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::BC3: return isSRGB ? VK_FORMAT_BC3_SRGB_BLOCK : VK_FORMAT_BC3_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::BC4: return VK_FORMAT_BC4_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::BC5: return VK_FORMAT_BC5_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::BC7: return isSRGB ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::ASTC: return isSRGB ? VK_FORMAT_ASTC_4x4_SRGB_BLOCK : VK_FORMAT_ASTC_4x4_UNORM_BLOCK;

        case compiler::TextureCompressionFormat::ETC: return isSRGB ? VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK : VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;

        default: return isSRGB ? VK_FORMAT_BC7_SRGB_BLOCK : VK_FORMAT_BC7_UNORM_BLOCK;
        }
    }
}

namespace compiler
{

    using namespace CMP;

    std::string WstringToString(const std::wstring_view InputView) noexcept
    {
        std::string Result(InputView.size(), 0);
        std::transform(InputView.begin(), InputView.end(), Result.begin(), [](const wchar_t wch) { return static_cast<char>(wch); });
        return Result;
    }

    CMP_BOOL g_bAbortCompression = false;  // If set true current compression will abort
    CMP_BOOL CompressionCallback(CMP_FLOAT fProgress, [[maybe_unused]] CMP_DWORD_PTR pUser1, [[maybe_unused]] CMP_DWORD_PTR pUser2)
    {
        std::printf("\rCompression progress = %3.0f", fProgress);
        return g_bAbortCompression;
    }

    CompilationResult TextureCompiler::Compile(const CompilerOptions& compileOptions)
    {
        CompilationResult result;
        options = compileOptions;

        //==========================
        // Load Source Texture
        //==========================
        CMP_Texture srcTexture = {};
        CMP_Texture destTexture = {};
        LoadSourceTexture(srcTexture);

        //==========================
        // Generate Thumbnail (64x64)
        //==========================
        GenerateThumbnail(srcTexture, result);

        //===================================
        // Initialize Compressed Destination
        // Set Compression Options
        //===================================
        CMP_CompressOptions cmpOptions = SetupCompressionOptions();

        //==========================
        // Compress Texture
        //==========================
        //Main compile function responsible for freeing data
        if (!CompressTexture(srcTexture, destTexture, cmpOptions))
        {
            stbi_image_free(srcTexture.pData);
            free(destTexture.pData);
            result.errors.push_back("Texture compression failure: " + options.general.inputPath.string());
            return result;
        }

        if (!SaveAsKTX2(destTexture, result))
        {
            stbi_image_free(srcTexture.pData);
            free(destTexture.pData);
            result.errors.push_back("Texture saving failure: " + options.general.inputPath.string());
            return result;
        }

        stbi_image_free(srcTexture.pData);
        free(destTexture.pData);

        return result;
    }

    CompilationResult TextureCompiler::CompileFromMemory(const EmbeddedTextureSource& source, const CompilerOptions& compileOptions)
    {
        CompilationResult result;
        options = compileOptions; // Set options for the helper functions

        //==========================
        // Load Source Texture
        //==========================
        CMP_Texture srcTexture = {};
        CMP_Texture destTexture = {};
        LoadSourceTextureFromMemory(source, srcTexture);

        //==========================
        // Generate Thumbnail (64x64)
        //==========================
        GenerateThumbnail(srcTexture, result, source.name);

        //===================================
        // Initialize Compressed Destination
        //===================================
        CMP_CompressOptions cmpOptions = SetupCompressionOptions();

        //==========================
        // Compress Texture
        //==========================
        if (!CompressTexture(srcTexture, destTexture, cmpOptions))
        {
            stbi_image_free(srcTexture.pData);
            free(destTexture.pData);
            result.errors.push_back("Texture compression failure: " + options.general.inputPath.string());
            result.success = false;
            return result;
        }

        if (!SaveAsKTX2(destTexture, result, source.name))
        {
            stbi_image_free(srcTexture.pData);
            free(destTexture.pData);
            result.errors.push_back("Texture saving failure: " + options.general.inputPath.string());
            result.success = false;
            return result;
        }

        stbi_image_free(srcTexture.pData);
        free(destTexture.pData);

        return result;
    }

    bool TextureCompiler::LoadSourceTexture(CMP_Texture& outTex)
    {
        int w, h, comp;
        unsigned char* pixels = stbi_load(options.general.inputPath.string().c_str(), &w, &h, &comp, 4);
        if (!pixels) return false;

        outTex.dwSize = sizeof(outTex);
        outTex.dwWidth = w;
        outTex.dwHeight = h;
        outTex.format = MapCMPFormat(options.texture.channelFormat);
        outTex.dwDataSize = w * h * 4;
        outTex.dwPitch = w * 4;
        outTex.pData = pixels;
        return true;
    }

    bool TextureCompiler::LoadSourceTextureFromMemory(const EmbeddedTextureSource& source, CMP_Texture& outTex)
    {
        int w{}, h{}, comp{};
        unsigned char* pixels = nullptr;

        if (source.compressedData)
        {
            pixels = stbi_load_from_memory(source.compressedData, static_cast<int>(source.compressedSize), &w, &h, &comp, 4);
        }
        else if (source.rawData)
        {
            // No STB needed for raw data, just copy it.
            w = source.width;
            h = source.height;
            comp = 4;
            size_t dataSize = static_cast<size_t>(w * h * 4);
            pixels = new unsigned char[dataSize];
            if (pixels)
            {
                memcpy(pixels, source.rawData, dataSize);
            }
        }

        if (!pixels)
        {
            return false;
        }

        outTex.dwSize = sizeof(outTex);
        outTex.dwWidth = w;
        outTex.dwHeight = h;
        outTex.format = MapCMPFormat(options.texture.channelFormat);
        outTex.dwDataSize = w * h * 4;
        outTex.dwPitch = w * 4;
        outTex.pData = pixels;
        return true;
    }

    CMP_CompressOptions TextureCompiler::SetupCompressionOptions()
    {
        CMP_CompressOptions opts = {};
        opts.dwSize = sizeof(opts);
        opts.dwnumThreads = std::thread::hardware_concurrency();
        opts.fquality = options.texture.quality; // normalized

        // Explicitly set source and destination formats for the encoder
        // This is required for some codecs (like ASTC) to properly identify the conversion path
        opts.SourceFormat = MapCMPFormat(options.texture.channelFormat);
        opts.DestFormat = MapCMPFormat(options.texture.compressionFormat);

        return opts;
    }

    bool TextureCompiler::CompressTextureASTC(CMP_Texture& src, CMP_Texture& dst)
    {
        // Use ARM's official astc-encoder for ASTC compression
        // This is much more reliable than Compressonator's ASTC implementation

        constexpr unsigned int BLOCK_X = 4;
        constexpr unsigned int BLOCK_Y = 4;
        constexpr unsigned int BLOCK_Z = 1;

        // Calculate output size: 16 bytes per block (128 bits)
        unsigned int blocksX = (src.dwWidth + BLOCK_X - 1) / BLOCK_X;
        unsigned int blocksY = (src.dwHeight + BLOCK_Y - 1) / BLOCK_Y;
        size_t outputSize = blocksX * blocksY * 16;  // 16 bytes per ASTC block

        // Setup destination texture
        dst.dwSize = sizeof(dst);
        dst.dwWidth = src.dwWidth;
        dst.dwHeight = src.dwHeight;
        dst.format = CMP_FORMAT_ASTC;
        dst.nBlockWidth = BLOCK_X;
        dst.nBlockHeight = BLOCK_Y;
        dst.nBlockDepth = BLOCK_Z;
        dst.dwDataSize = static_cast<CMP_DWORD>(outputSize);
        dst.pData = (CMP_BYTE*)malloc(outputSize);

        if (!dst.pData)
        {
            std::cout << "ASTC: Failed to allocate output buffer\n";
            return false;
        }

        // Initialize ASTC encoder config
        astcenc_config config{};
        astcenc_profile profile = options.texture.isSRGB ? ASTCENC_PRF_LDR_SRGB : ASTCENC_PRF_LDR;

        // Map quality (0-1) to ASTC preset
        float quality = options.texture.quality;
        float astcQuality;
        if (quality < 0.2f)
            astcQuality = ASTCENC_PRE_FASTEST;
        else if (quality < 0.4f)
            astcQuality = ASTCENC_PRE_FAST;
        else if (quality < 0.7f)
            astcQuality = ASTCENC_PRE_MEDIUM;
        else if (quality < 0.9f)
            astcQuality = ASTCENC_PRE_THOROUGH;
        else
            astcQuality = ASTCENC_PRE_EXHAUSTIVE;

        astcenc_error status = astcenc_config_init(
            profile, BLOCK_X, BLOCK_Y, BLOCK_Z,
            astcQuality, 0, &config);

        if (status != ASTCENC_SUCCESS)
        {
            std::cout << "ASTC config_init failed: " << status << "\n";
            return false;
        }

        // Allocate compression context
        astcenc_context* context = nullptr;
        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 1;

        status = astcenc_context_alloc(&config, threadCount, &context);
        if (status != ASTCENC_SUCCESS)
        {
            std::cout << "ASTC context_alloc failed: " << status << "\n";
            return false;
        }

        // Setup input image (RGBA 8-bit)
        astcenc_image image{};
        image.dim_x = src.dwWidth;
        image.dim_y = src.dwHeight;
        image.dim_z = 1;
        image.data_type = ASTCENC_TYPE_U8;

        // astcenc_image expects an array of slice pointers
        void* slicePtr = src.pData;
        image.data = &slicePtr;

        // Setup swizzle (RGBA -> RGBA)
        astcenc_swizzle swizzle{ ASTCENC_SWZ_R, ASTCENC_SWZ_G, ASTCENC_SWZ_B, ASTCENC_SWZ_A };

        // Compress
        status = astcenc_compress_image(context, &image, &swizzle, dst.pData, outputSize, 0);

        astcenc_context_free(context);

        if (status != ASTCENC_SUCCESS)
        {
            std::cout << "ASTC compress_image failed: " << status << "\n";
            return false;
        }

        std::cout << "ASTC compression successful (" << src.dwWidth << "x" << src.dwHeight << ")\n";
        return true;
    }

    bool TextureCompiler::CompressTexture(CMP_Texture& src, CMP_Texture& dst, const CMP_CompressOptions& opts)
    {
        // Use ARM's astc-encoder for ASTC (much more reliable than Compressonator)
        if (options.texture.compressionFormat == TextureCompressionFormat::ASTC)
        {
            return CompressTextureASTC(src, dst);
        }

        // Use Compressonator for BC and ETC formats
        dst.dwSize = sizeof(dst);
        dst.dwWidth = src.dwWidth;
        dst.dwHeight = src.dwHeight;
        dst.format = MapCMPFormat(options.texture.compressionFormat);

        dst.dwDataSize = CMP_CalculateBufferSize(&dst);
        dst.pData = (CMP_BYTE*)malloc(dst.dwDataSize);

        CMP_ERROR status = CMP_ConvertTexture(&src, &dst, &opts, CompressionCallback);
        if (status != CMP_OK)
        {
            std::cout << "TextureCompiler::CompressTexture ERROR CODE: " << status << "\n";
            return false;
        }
        return true;
    }

    bool TextureCompiler::SaveAsKTX2(const CMP_Texture& dst, CompilationResult& compilationResult, const std::string& filename)
    {
        ktxTextureCreateInfo createInfo{};
        createInfo.vkFormat = MapVkFormat(options.texture.compressionFormat, options.texture.isSRGB);
        createInfo.baseWidth = dst.dwWidth;
        createInfo.baseHeight = dst.dwHeight;
        createInfo.baseDepth = 1;
        createInfo.numDimensions = 2;
        createInfo.numLevels = options.texture.generateMipmaps ? options.texture.mipCount : 1;
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = options.texture.generateMipmaps ? KTX_TRUE : KTX_FALSE;

        ktxTexture2* kTexture;
        ktx_error_code_e result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);
        if (result != KTX_SUCCESS)
        {
            std::cout << "KTX2 Create ERROR: " << result << "\n";
            return false;
        }

        ktxTexture_SetImageFromMemory(
            reinterpret_cast<ktxTexture*>(kTexture),
            // mip, layer, face
            0, 0, 0,
            dst.pData, dst.dwDataSize
        );

        std::string outputFilename = options.general.inputPath.stem().string() + ".ktx2";
        if (!filename.empty())
        {
            outputFilename = filename + ".ktx2";
        }

        std::filesystem::path outputPath = options.general.outputPath / outputFilename;
        //std::filesystem::path outputPath = assetOutputDir / outputFilename;

        ktx_error_code_e writeResult = ktxTexture_WriteToNamedFile(reinterpret_cast<ktxTexture*>(kTexture), outputPath.string().c_str());

        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(kTexture));

        if (writeResult != KTX_SUCCESS)
        {
            std::cout << "KTX2 Saving ERROR: " << writeResult << "\n";
            return false;
        }
        else
        {
            std::cout << "Successfully saved: " << outputPath << "\n";
        }
        compilationResult.createdTextureFiles.push_back(outputPath);

        // Write .meta sidecar file with source tracking
        Resource::AssetMetadata texMeta;
        texMeta.assetType = AssetFormat::AssetType::Texture;
        texMeta.sourcePath = GetRelativeSourcePath(options.general.inputPath, options.general.assetsRoot);
        texMeta.platform = GetPlatformName(options.general.platform);
        // Only get source timestamp if the file actually exists (not for embedded textures)
        if (std::filesystem::exists(options.general.inputPath))
        {
            texMeta.sourceTimestamp = static_cast<uint64_t>(
                std::filesystem::last_write_time(options.general.inputPath).time_since_epoch().count());
        }
        texMeta.compiledTimestamp = static_cast<uint64_t>(
            std::chrono::system_clock::now().time_since_epoch().count());
        texMeta.formatVersion = 2;  // KTX2 format version
        // Set thumbnail path (relative filename only since it's in the same directory)
        if (!m_lastThumbnailPath.empty())
        {
            texMeta.thumbnailPath = m_lastThumbnailPath.filename().string();
            m_lastThumbnailPath.clear();  // Reset for next texture
        }
        texMeta.saveToFile(Resource::AssetMetadata::getMetaPath(outputPath));

        return writeResult == KTX_SUCCESS;
    }

    ktxTextureCreateInfo TextureCompiler::SetupKtxCreateInfo(const CMP_Texture& dest)
    {
        ktxTextureCreateInfo info{};
        info.vkFormat = MapVkFormat(options.texture.compressionFormat, options.texture.isSRGB);
        info.baseWidth = dest.dwWidth;
        info.baseHeight = dest.dwHeight;
        info.baseDepth = 1;
        info.numDimensions = 2;
        info.numLevels = options.texture.generateMipmaps ? options.texture.mipCount : 1;
        info.numLayers = 1;
        info.numFaces = 1;
        info.isArray = KTX_FALSE;
        info.generateMipmaps = options.texture.generateMipmaps ? KTX_TRUE : KTX_FALSE;
        return info;
    }

    bool TextureCompiler::GenerateThumbnail(const CMP_Texture& srcTexture, CompilationResult& result, const std::string& filename)
    {
        // Skip if source is already smaller than thumbnail size
        if (srcTexture.dwWidth <= THUMBNAIL_SIZE && srcTexture.dwHeight <= THUMBNAIL_SIZE)
        {
            std::cout << "Skipping thumbnail generation - source already small enough\n";
            return true;
        }

        // Resize source to 64x64 thumbnail
        std::vector<unsigned char> thumbPixels(THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4);

        // Use stb_image_resize2 for high-quality downsampling
        stbir_resize_uint8_srgb(
            static_cast<const unsigned char*>(srcTexture.pData),
            static_cast<int>(srcTexture.dwWidth),
            static_cast<int>(srcTexture.dwHeight),
            static_cast<int>(srcTexture.dwPitch),
            thumbPixels.data(),
            THUMBNAIL_SIZE,
            THUMBNAIL_SIZE,
            THUMBNAIL_SIZE * 4,
            STBIR_RGBA
        );

        // Create thumbnail CMP_Texture
        CMP_Texture thumbSrc = {};
        thumbSrc.dwSize = sizeof(thumbSrc);
        thumbSrc.dwWidth = THUMBNAIL_SIZE;
        thumbSrc.dwHeight = THUMBNAIL_SIZE;
        thumbSrc.format = CMP_FORMAT_RGBA_8888;
        thumbSrc.dwDataSize = THUMBNAIL_SIZE * THUMBNAIL_SIZE * 4;
        thumbSrc.dwPitch = THUMBNAIL_SIZE * 4;
        thumbSrc.pData = thumbPixels.data();

        // Compress thumbnail (use same format as main texture)
        CMP_Texture thumbDst = {};
        CMP_CompressOptions cmpOptions = SetupCompressionOptions();

        if (!CompressTexture(thumbSrc, thumbDst, cmpOptions))
        {
            std::cout << "Thumbnail compression failed\n";
            return false;
        }

        // Save thumbnail as KTX2
        std::string thumbFilename = options.general.inputPath.stem().string() + "_thumb";
        if (!filename.empty())
        {
            thumbFilename = filename + "_thumb";
        }

        // Create KTX2 for thumbnail
        ktxTextureCreateInfo createInfo{};
        createInfo.vkFormat = MapVkFormat(options.texture.compressionFormat, options.texture.isSRGB);
        createInfo.baseWidth = THUMBNAIL_SIZE;
        createInfo.baseHeight = THUMBNAIL_SIZE;
        createInfo.baseDepth = 1;
        createInfo.numDimensions = 2;
        createInfo.numLevels = 1;  // No mipmaps for thumbnails
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = KTX_FALSE;
        createInfo.generateMipmaps = KTX_FALSE;

        ktxTexture2* kTexture;
        ktx_error_code_e ktxResult = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);
        if (ktxResult != KTX_SUCCESS)
        {
            std::cout << "Thumbnail KTX2 Create ERROR: " << ktxResult << "\n";
            free(thumbDst.pData);
            return false;
        }

        ktxTexture_SetImageFromMemory(
            reinterpret_cast<ktxTexture*>(kTexture),
            0, 0, 0,
            thumbDst.pData, thumbDst.dwDataSize
        );

        std::filesystem::path outputPath = options.general.outputPath / (thumbFilename + ".ktx2");
        ktx_error_code_e writeResult = ktxTexture_WriteToNamedFile(
            reinterpret_cast<ktxTexture*>(kTexture),
            outputPath.string().c_str()
        );

        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(kTexture));
        free(thumbDst.pData);

        if (writeResult != KTX_SUCCESS)
        {
            std::cout << "Thumbnail KTX2 Saving ERROR: " << writeResult << "\n";
            return false;
        }

        std::cout << "Generated thumbnail: " << outputPath << "\n";
        result.createdTextureFiles.push_back(outputPath);
        m_lastThumbnailPath = outputPath;  // Store for meta file
        return true;
    }

    bool TextureCompiler::SaveThumbnailKTX2(const std::vector<uint8_t>& rgbaPixels, uint32_t size, const std::filesystem::path& outputPath)
    {
        if (rgbaPixels.size() < size * size * 4) {
            std::cerr << "[TextureCompiler] SaveThumbnailKTX2: Invalid pixel data size\n";
            return false;
        }

        // Create KTX2 texture info
        ktxTextureCreateInfo createInfo = {};
        createInfo.glInternalformat = 0;  // Not used for Vulkan
        createInfo.vkFormat = VK_FORMAT_R8G8B8A8_UNORM;
        createInfo.baseWidth = size;
        createInfo.baseHeight = size;
        createInfo.baseDepth = 1;
        createInfo.numDimensions = 2;
        createInfo.numLevels = 1;
        createInfo.numLayers = 1;
        createInfo.numFaces = 1;
        createInfo.isArray = false;
        createInfo.generateMipmaps = false;

        ktxTexture2* kTexture = nullptr;
        KTX_error_code result = ktxTexture2_Create(&createInfo, KTX_TEXTURE_CREATE_ALLOC_STORAGE, &kTexture);
        if (result != KTX_SUCCESS) {
            std::cerr << "[TextureCompiler] SaveThumbnailKTX2: Failed to create KTX texture: " << result << "\n";
            return false;
        }

        // Copy pixel data
        memcpy(kTexture->pData, rgbaPixels.data(), size * size * 4);

        // Compress with BasisU for smaller file size (optional - can use uncompressed for simplicity)
        ktx_uint32_t numThreads = std::thread::hardware_concurrency();
        ktxBasisParams basisParams = {};
        basisParams.structSize = sizeof(ktxBasisParams);
        basisParams.threadCount = numThreads;
        basisParams.uastc = KTX_TRUE;  // Use UASTC for quality
        basisParams.qualityLevel = 128; // Medium quality for thumbnails

        result = ktxTexture2_CompressBasisEx(kTexture, &basisParams);
        if (result != KTX_SUCCESS) {
            // If compression fails, save uncompressed
            std::cerr << "[TextureCompiler] SaveThumbnailKTX2: BasisU compression failed, saving uncompressed\n";
        }

        // Write to file
        result = ktxTexture_WriteToNamedFile(
            reinterpret_cast<ktxTexture*>(kTexture),
            outputPath.string().c_str()
        );

        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(kTexture));

        if (result != KTX_SUCCESS) {
            std::cerr << "[TextureCompiler] SaveThumbnailKTX2: Failed to write file: " << result << "\n";
            return false;
        }

        return true;
    }
}
