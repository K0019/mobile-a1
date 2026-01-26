/******************************************************************************/
/*!
\file   CompileOptions.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/06/2025

\author Rocky Sutarius (100%)
\par    email: rocky.sutarius\@digipen.edu
\par    DigiPen login: rocky.sutarius

\brief
Options for compiling.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include <source_location>
#include <string>
#include <array>
#include <chrono>
#include <filesystem>


namespace compiler
{
    struct CompilationResult
    {
        bool success = true;

        // Paths to the newly created files.
        std::vector<std::filesystem::path> createdMeshFiles;
        std::vector<std::filesystem::path> createdMaterialFiles;
        std::vector<std::filesystem::path> createdTextureFiles;
        std::vector<std::filesystem::path> createdAnimationFiles;

        std::vector<std::string> warnings;
        std::vector<std::string> errors;
    };

    // This entire section is useless right now.
    // Thank you for the inspiration though xtexture.plugin  \o LIONant o/
    enum BUILD_PLATFORM : std::uint8_t
    {
        WINDOWS,
        ANDROID,
        COUNT
    };

    inline const char* GetPlatformName(BUILD_PLATFORM platform)
    {
        switch (platform)
        {
            case BUILD_PLATFORM::WINDOWS: return "windows";
            case BUILD_PLATFORM::ANDROID: return "android";
            default: return "unknown";
        }
    }

    // Get relative path from assets root (for portable metadata)
    inline std::string GetRelativeSourcePath(const std::filesystem::path& inputPath, const std::filesystem::path& assetsRoot)
    {
        // Use lexically_relative to get a portable relative path
        auto relative = inputPath.lexically_relative(assetsRoot);
        // Convert to forward slashes for cross-platform compatibility
        return relative.generic_string();
    }

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
        ARGB_8888,
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

        ASTC,
        ETC
    };

    // Options
    struct GeneralOptions
    {
        std::filesystem::path assetsRoot;   // our case would usually be ../../../Assets (in editor)
        std::filesystem::path inputPath;
        std::filesystem::path outputPath;
        BUILD_PLATFORM platform = BUILD_PLATFORM::WINDOWS;  // Target platform for compilation
    };

    struct MeshOptions
    {
        bool optimize = true;
        OPTIMIZATION_TYPE optimization = OPTIMIZATION_TYPE::O1;
        bool generateNormals = true;
        bool generateTangents = true;
        bool calculateBounds = true;
        bool flipUVs = true;
    };

    struct TextureOptions
    {
        TextureChannelFormat channelFormat = TextureChannelFormat::RGBA_8888;
        TextureCompressionFormat compressionFormat = TextureCompressionFormat::BC7;
        bool isSRGB = false;

        float quality = 0.05f; // 0.0f-1.0f

        bool generateMipmaps = false;
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

}
