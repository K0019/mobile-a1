#pragma once
#include <source_location>
#include <string>
#include <array>
#include <chrono>
#include <filesystem>
#include "CmdlineParser.h"

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

    /*
    class CompilerBase
    {
    public:
        enum MESSAGE
        {
            WARNING,
            ERROR,
            INFO,
            OK
        };


        virtual                 ~CompilerBase(void) = default;

        //MESSAGE                 Parse(int argc, const char* argv[]);
        //virtual MESSAGE         Compile(const CompileOptions& options) = 0;
        //virtual void            SetupCommandLine();

    protected:
        //virtual MESSAGE         OnCompile() = 0;

        void                    StartTimer();
        void                    EndTimer();
        void                    LogMessage(MESSAGE err, std::string message);



        //Parser parser;
        //
        ////Parsed Options
        //using platform_list = std::array<Platform, static_cast<int>(BUILD_PLATFORM::COUNT)>;
        //DEBUG_TYPE                                              debugType{ DEBUG_TYPE::D0 };
        //OPTIMIZATION_TYPE                                       optimizationType{ OPTIMIZATION_TYPE::O1 };
        //std::chrono::steady_clock::time_point                   timer{};
        //std::wstring                                            descriptorPath{}; // Path to of the descriptor that we have been asked to compile
        //std::wstring                                            outputPath{};
        //platform_list                                           targets{};

        //FILE*                                                   pLogFile{};

        ////???????
        //std::wstring                                            resourceType{};
        //std::wstring                                            resourcePartialPath{};
        //std::wstring                                            resourceLogPath{};
    };
    */
}
