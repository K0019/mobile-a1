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

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TextureCompiler.h"

#include <stb_image.h>

#include <filesystem>
#include <cassert>
#include <thread>
#include <iostream>

namespace
{
    CMP_FORMAT MapCMPFormat(compiler::TextureChannelFormat fmt)
    {
        switch (fmt)
        {
        case compiler::TextureChannelFormat::RGBA_8888: return CMP_FORMAT_RGBA_8888;
        case compiler::TextureChannelFormat::RGBA_8888_S: return CMP_FORMAT_RGBA_8888_S;
        case compiler::TextureChannelFormat::ARGB_8888: return CMP_FORMAT_ARGB_8888;
        case compiler::TextureChannelFormat::ARGB_8888_S: return CMP_FORMAT_ARGB_8888_S;
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
        case compiler::TextureCompressionFormat::BC1: return CMP_FORMAT_BC1;
        case compiler::TextureCompressionFormat::BC3: return CMP_FORMAT_BC3;
        case compiler::TextureCompressionFormat::BC4: return CMP_FORMAT_BC4;
        case compiler::TextureCompressionFormat::BC5: return CMP_FORMAT_BC5;
        case compiler::TextureCompressionFormat::BC7: return CMP_FORMAT_BC7;
        case compiler::TextureCompressionFormat::ASTC: return CMP_FORMAT_ASTC;
        case compiler::TextureCompressionFormat::ETC: return CMP_FORMAT_ETC_RGB;
            //uncompressed?
        default: return CMP_FORMAT_BC7;
        }
    }

    VkFormat MapVkFormat(compiler::TextureCompressionFormat fmt)
    {
        switch (fmt)
        {
        case compiler::TextureCompressionFormat::BC1: return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::BC3: return VK_FORMAT_BC3_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::BC4: return VK_FORMAT_BC4_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::BC5: return VK_FORMAT_BC5_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::BC7: return VK_FORMAT_BC7_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::ASTC: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
        case compiler::TextureCompressionFormat::ETC: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
        default: return VK_FORMAT_BC7_UNORM_BLOCK;
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

    bool TextureCompiler::Compile(const CompilerOptions& compileOptions)
    {
        options = compileOptions;

        //==========================
        // Load Source Texture
        //==========================
        CMP_Texture srcTexture = {};
        CMP_Texture destTexture = {};
        LoadSourceTexture(srcTexture);

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
            return false;
        }

        if (!SaveAsKTX2(destTexture))
        {
            stbi_image_free(srcTexture.pData);
            free(destTexture.pData);

            return false;
        }

        stbi_image_free(srcTexture.pData);
        free(destTexture.pData);

        return true;
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
        outTex.pData = pixels;
        return true;
    }

    CMP_CompressOptions TextureCompiler::SetupCompressionOptions()
    {
        CMP_CompressOptions opts = {};
        opts.dwSize = sizeof(opts);
        opts.dwnumThreads = std::thread::hardware_concurrency();
        opts.fquality = options.texture.quality; // normalized
        return opts;
    }

    bool TextureCompiler::CompressTexture(CMP_Texture& src, CMP_Texture& dst, const CMP_CompressOptions& opts)
    {
        dst.dwSize = sizeof(dst);
        dst.dwWidth = src.dwWidth;
        dst.dwHeight = src.dwHeight;
        dst.format = MapCMPFormat(options.texture.compressionFormat);
        dst.dwDataSize = CMP_CalculateBufferSize(&dst);
        dst.pData = (CMP_BYTE*)malloc(dst.dwDataSize);

        CMP_ERROR status = CMP_ConvertTexture(&src, &dst, &opts, CompressionCallback);
        if (status != CMP_OK)
        {
            std::cout << "ERROR CODE: " << status << "\n";
            return false;
        }
        return true;
    }

    bool TextureCompiler::SaveAsKTX2(const CMP_Texture& dst)
    {
        ktxTextureCreateInfo createInfo{};
        createInfo.vkFormat = MapVkFormat(options.texture.compressionFormat);
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
        std::filesystem::path outputPath = options.general.outputPath / outputFilename;

        ktx_error_code_e writeResult = ktxTexture_WriteToNamedFile(reinterpret_cast<ktxTexture*>(kTexture), outputPath.string().c_str());

        ktxTexture_Destroy(reinterpret_cast<ktxTexture*>(kTexture));

        if (writeResult != KTX_SUCCESS)
        {
            std::cout << "KTX2 Saving ERROR: " << writeResult << "\n";
            return false;
        }
        return writeResult == KTX_SUCCESS;
    }

    ktxTextureCreateInfo TextureCompiler::SetupKtxCreateInfo(const CMP_Texture& dest)
    {
        ktxTextureCreateInfo info{};
        info.vkFormat = MapVkFormat(options.texture.compressionFormat);
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
}
