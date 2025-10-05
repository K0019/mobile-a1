#pragma once
#include <source_location>
#include <string>
#include <array>
#include <chrono>
#include <filesystem>


namespace compiler
{

    enum BUILD_PLATFORM : std::uint8_t
    {
        WINDOWS,
        ANDROID,
        COUNT
    };

    enum OPTIMIZATION_TYPE
    {
        O0,                // Compiles the asset as fast as possible no real optimization
        O1,               // (Default) Build with optimizations
        Oz,                // Take all the time you like to optimize this resource
    };

    enum DEBUG_TYPE
    {
        D0,                // (Default) Compiles with some debug
        D1,                // Compiles with extra debug information
        Dz                // Runs the debug version of the compiler and such... max debug level
    };

    struct Platform
    {
        bool                    toBuild{ false };            // If we need to build for this platform
        std::filesystem::path   dataPath{};                 // This is where the compiler need to drop all the compiled data
    };



    enum TextureChannelFormat
    {
        RGBA_8888,
        RGBA_8888_S,
        ARGB_8888,
        ARGB_8888_S,
        RGBA_16F,
        ARGB_16F,
        ARGB_32F,
        RGBA_1010102
    };
    enum TextureCompressionFormat
    {
        UNCOMPRESSED,
        BC1,
        BC3,
        BC4,
        BC5,
        BC7,

        //???????
        ASTC,
        ETC
    };
    enum TextureWrapMode
    {
        CLAMP_TO_EDGE,
        WRAP,
        MIRROR
    };
    enum TextureAlphaMode
    {
        NONE,
        MASK,
        BLEND
    };


    // Options
    struct GeneralOptions
    {
        std::filesystem::path inputPath;
        std::filesystem::path outputPath;
        bool forceRecompile = false;
    };

    struct MeshOptions
    {
        bool optimize = true;
        OPTIMIZATION_TYPE optimization = OPTIMIZATION_TYPE::O1;
        bool generateNormals = true;
        bool generateTangents = true;
        bool calculateBounds = true;
        bool flipUVs = false;
    };

    struct TextureOptions
    {
        TextureChannelFormat channelFormat = TextureChannelFormat::RGBA_8888;
        TextureCompressionFormat compressionFormat = TextureCompressionFormat::BC7;
        TextureWrapMode wrapMode;
        TextureAlphaMode alphaMode;

        float quality; // 0.0f-1.0f

        bool generateMipmaps;
        int mipCount = 1;

        // Tiling and Wrapping requires their own settings
        //
    };

    struct CompilerOptions
    {
        GeneralOptions general;
        MeshOptions mesh;
        TextureOptions texture;
    };




    //struct CommonCompileOptions
    //{
    //    std::filesystem::path inputPath;
    //    std::filesystem::path outputPath;
    //    OPTIMIZATION_TYPE optimization = OPTIMIZATION_TYPE::O1;
    //    DEBUG_TYPE debug = DEBUG_TYPE::D0;

    //    using platform_list = std::array<Platform, static_cast<int>(BUILD_PLATFORM::COUNT)>;
    //    platform_list targets{};
    //};

}
