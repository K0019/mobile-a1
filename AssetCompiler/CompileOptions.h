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


    struct CommonCompileOptions
    {
        std::filesystem::path inputPath;
        std::filesystem::path outputPath;
        OPTIMIZATION_TYPE optimization = OPTIMIZATION_TYPE::O1;
        DEBUG_TYPE debug = DEBUG_TYPE::D0;

        using platform_list = std::array<Platform, static_cast<int>(BUILD_PLATFORM::COUNT)>;
        platform_list targets{};
    };

}
