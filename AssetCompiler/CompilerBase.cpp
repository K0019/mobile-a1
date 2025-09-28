//#include "CompilerBase.h"
//#include "CmdlineParser.h"
//#include <filesystem>
//#include <iostream>
//
//namespace compiler
//{
//    /*
//    void CompilerBase::SetupCommandLine()
//    {
//        // Required arguments
//        parser.AddOption("input", "Input descriptor file path", true, 1);
//
//        parser.AddOption("output", "Output directory path", true, 1);
//
//        // Optional arguments
//        parser.AddOption("target", "Target platform(s): WINDOWS, ANDROID", false, 1);
//
//        parser.AddOption("optimization", "Optimization level: O0, O1, Oz (default: O1)", false, 1);
//
//        parser.AddOption("debug", "Debug level: D0, D1, Dz (default: D0)", false, 1);
//
//        parser.AddOption("log", "Log file path (optional)", false, 1);
//
//        parser.AddOption("help", "Show this help message", false, 0);
//    }
//
//    CompilerBase::MESSAGE CompilerBase::Parse(int argc, const char* argv[])
//    {
//        LogMessage(MESSAGE::INFO, " Parsing Input...");
//
//        if (!parser.Parse(argc, argv))
//        {
//            LogMessage(MESSAGE::ERROR, "parser.Parse failed!\n");
//            return MESSAGE::ERROR;
//        }
//
//        // Required: Input file, descriptor
//        if (parser.HasOption("input"))
//        {
//            auto input = parser.GetOption<std::string>("input");
//            if (input.has_value())
//            {
//                descriptorPath = std::filesystem::path(input.value()).wstring();
//            }
//        }
//        else
//        {
//            LogMessage(MESSAGE::ERROR, "required -input missing!\n");
//            return MESSAGE::ERROR;
//        }
//
//        // Required: Output path
//        if (parser.HasOption("output"))
//        {
//            auto output = parser.GetOption<std::string>("output");
//            if (output.has_value())
//            {
//                outputPath = std::filesystem::path(output.value()).wstring();
//            }
//        }
//        else
//        {
//            LogMessage(MESSAGE::ERROR, "required -output missing!\n");
//            return MESSAGE::ERROR;
//        }
//
//        // Target Platform, windows default
//        std::string targetStr;
//        if (parser.HasOption("target"))
//        {
//            auto platforms = parser.GetOptionArgs("target");
//            for (auto platformStr : platforms)
//            {
//                if (platformStr == "WINDOWS")
//                {
//                    targets[BUILD_PLATFORM::WINDOWS].toBuild = true;
//                }
//                if (platformStr == "ANDROID")
//                {
//                    targets[BUILD_PLATFORM::ANDROID].toBuild = true;
//                }
//            }
//        }
//        else
//        {
//            targets[BUILD_PLATFORM::WINDOWS].toBuild = true;
//            targets[BUILD_PLATFORM::ANDROID].toBuild = false;
//        }
//
//        // Optimization Level
//        if (parser.HasOption("optimization"))
//        {
//            std::string optStr;
//            if (parser.HasOption("optimization"))
//            {
//                auto opt = parser.GetOption<std::string>("optimization");
//                if (opt.has_value()) optStr = opt.value();
//            }
//            else
//            {
//                auto opt = parser.GetOption<std::string>("opt");
//                if (opt.has_value()) optStr = opt.value();
//            }
//
//            if (optStr == "O0")      optimizationType = OPTIMIZATION_TYPE::O0;
//            else if (optStr == "O1") optimizationType = OPTIMIZATION_TYPE::O1;
//            else if (optStr == "Oz") optimizationType = OPTIMIZATION_TYPE::Oz;
//        }
//
//        // Optional: Debug level
//        if (parser.HasOption("debug"))
//        {
//            std::string debugStr;
//            if (parser.HasOption("debug"))
//            {
//                auto debug = parser.GetOption<std::string>("debug");
//                if (debug.has_value()) debugStr = debug.value();
//            }
//            else
//            {
//                auto debug = parser.GetOption<std::string>("d");
//                if (debug.has_value()) debugStr = debug.value();
//            }
//
//            if (debugStr == "D0")      debugType = DEBUG_TYPE::D0;
//            else if (debugStr == "D1") debugType = DEBUG_TYPE::D1;
//            else if (debugStr == "Dz") debugType = DEBUG_TYPE::Dz;
//        }
//
//
//        return MESSAGE::OK;
//    }
//    */
//
//    /*
//    CompilerBase::MESSAGE CompilerBase::Compile(const CompileOptions& options)
//    {
//        timer = std::chrono::steady_clock::now();
//
//        //
//        //Create Log Folder
//        //
//
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//        LogMessage(MESSAGE::INFO, " Start Compilation");
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//
//
//
//
//        if (auto err = OnCompile(); err != CompilerBase::MESSAGE::OK)
//        {
//            return err;
//        }
//
//
//
//
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//        LogMessage(MESSAGE::INFO, std::format("Compilation Time: {:.3f}s", std::chrono::duration<float>(std::chrono::steady_clock::now() - timer).count()));
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//
//        return CompilerBase::MESSAGE::OK;
//    }
//    */
//
///*
//    void CompilerBase::StartTimer()
//    {
//        timer = std::chrono::steady_clock::now();
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//        LogMessage(MESSAGE::INFO, " Start Compilation");
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//    }
//
//    void CompilerBase::EndTimer()
//    {
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//        LogMessage(MESSAGE::INFO, std::format("Compilation Time: {:.3f}s", std::chrono::duration<float>(std::chrono::steady_clock::now() - timer).count()));
//        LogMessage(MESSAGE::INFO, "------------------------------------------------------------------");
//    }
//
//    void CompilerBase::LogMessage(CompilerBase::MESSAGE err, std::string message)
//    {
//        std::string log;
//        //Just print to cout
//        switch (err)
//        {
//        case MESSAGE::WARNING:
//            log += "[WARNING] ";
//            break;
//        case MESSAGE::ERROR:
//            log += "[ERROR] ";
//            break;
//        case MESSAGE::INFO:
//            log += "[INFO] ";
//            break;
//        }
//        log += message;
//
//        std::cout << log << "\n";
//
//        //
//        //write to file if debug level blah blah
//        //
//    }
//*/
//
//}