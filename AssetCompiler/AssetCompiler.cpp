#include "CompileOptions.h"
#include "SceneCompiler.h"
#include "TextureCompiler.h"
#include "ThumbnailRenderer.h"
#include "CmdlineParser.h"
#include "ProgressReporter.h"
#include "resource/asset_metadata.h"
#include "resource/asset_formats/mesh_format.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <fstream>
#include <chrono>
#include <ctime>
#include <windows.h>
#include <DbgHelp.h>
#include <sstream>

#pragma comment(lib, "DbgHelp.lib")


using namespace compiler;


std::filesystem::path g_ManifestPath;
std::filesystem::path g_InputPath;
std::string g_StartTime;

// ----- Helpers ----- //
// Stack trace capture for debugging
std::string CaptureStackTrace(CONTEXT* context = nullptr)
{
    std::stringstream ss;

    HANDLE process = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();

    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_UNDNAME);
    if (!SymInitialize(process, NULL, TRUE))
    {
        ss << "[Stack trace unavailable - SymInitialize failed: " << GetLastError() << "]\n";
        return ss.str();
    }

    CONTEXT ctx;
    if (context)
    {
        ctx = *context;
    }
    else
    {
        RtlCaptureContext(&ctx);
    }

    STACKFRAME64 frame = {};
#ifdef _M_X64
    DWORD machineType = IMAGE_FILE_MACHINE_AMD64;
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
#else
    DWORD machineType = IMAGE_FILE_MACHINE_I386;
    frame.AddrPC.Offset = ctx.Eip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Ebp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Esp;
    frame.AddrStack.Mode = AddrModeFlat;
#endif

    ss << "\n========== STACK TRACE ==========\n";

    char symbolBuffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO symbol = (PSYMBOL_INFO)symbolBuffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    int frameNum = 0;
    while (StackWalk64(machineType, process, thread, &frame, &ctx,
        NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL))
    {
        if (frame.AddrPC.Offset == 0)
            break;

        DWORD64 displacement64 = 0;
        DWORD displacement = 0;

        ss << "[" << frameNum++ << "] ";

        if (SymFromAddr(process, frame.AddrPC.Offset, &displacement64, symbol))
        {
            ss << symbol->Name;

            if (SymGetLineFromAddr64(process, frame.AddrPC.Offset, &displacement, &line))
            {
                ss << " (" << line.FileName << ":" << line.LineNumber << ")";
            }
        }
        else
        {
            ss << "0x" << std::hex << frame.AddrPC.Offset << std::dec;
        }

        ss << "\n";

        if (frameNum > 30) // Limit stack depth
        {
            ss << "... (truncated)\n";
            break;
        }
    }

    ss << "=================================\n";

    SymCleanup(process);
    return ss.str();
}

// Results, crash handling
std::string GetCurrentTimeStr()
{
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char buffer[26];
    ctime_s(buffer, sizeof(buffer), &now_time);
    std::string timeStr(buffer);
    // Remove newline char that ctime adds
    if (!timeStr.empty() && timeStr.back() == '\n') timeStr.pop_back();
    return timeStr;
}

void WriteManifest(const CompilationResult& result, bool crashed = false)
{
    rapidjson::StringBuffer sb;
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(sb);

    writer.StartObject();
    writer.Key("input_file");
    writer.String(g_InputPath.string().c_str());

    writer.Key("timestamp");
    writer.String(g_StartTime.c_str());

    writer.Key("status");
    if (crashed) writer.String("CRASHED");
    else writer.String(result.success ? "SUCCESS" : "FAILURE");


    auto WritePathArray = [&](const char* key, const std::vector<std::filesystem::path>& paths) {
        writer.Key(key);
        writer.StartArray();
        for (const auto& p : paths) writer.String(p.generic_string().c_str());
        writer.EndArray();
    };

    WritePathArray("meshes", result.createdMeshFiles);
    WritePathArray("textures", result.createdTextureFiles);
    WritePathArray("materials", result.createdMaterialFiles);
    WritePathArray("animations", result.createdAnimationFiles);

    writer.Key("errors");
    writer.StartArray();
    for (const auto& err : result.errors) writer.String(err.c_str());
    if (crashed) writer.String("CRITICAL PROCESS EXCEPTION (Access Violation/Segfault)");
    writer.EndArray();

    writer.Key("warnings");
    writer.StartArray();
    for (const auto& w : result.warnings) writer.String(w.c_str());
    writer.EndArray();



    writer.EndObject();

    if (!g_ManifestPath.parent_path().empty())
        std::filesystem::create_directories(g_ManifestPath.parent_path());
    std::ofstream file(g_ManifestPath);
    
    file << sb.GetString();
}

LONG WINAPI UnhandledException(PEXCEPTION_POINTERS pExceptionInfo)
{
    std::cerr << "\n[CRITICAL] ========================================\n";
    std::cerr << "[CRITICAL] UNHANDLED EXCEPTION!\n";

    if (pExceptionInfo && pExceptionInfo->ExceptionRecord)
    {
        DWORD code = pExceptionInfo->ExceptionRecord->ExceptionCode;
        std::cerr << "[CRITICAL] Exception code: 0x" << std::hex << code << std::dec;

        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            std::cerr << " (ACCESS_VIOLATION)";
            if (pExceptionInfo->ExceptionRecord->NumberParameters >= 2)
            {
                ULONG_PTR type = pExceptionInfo->ExceptionRecord->ExceptionInformation[0];
                ULONG_PTR addr = pExceptionInfo->ExceptionRecord->ExceptionInformation[1];
                std::cerr << "\n[CRITICAL] " << (type == 0 ? "Read" : "Write")
                          << " violation at address: 0x" << std::hex << addr << std::dec;
            }
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED: std::cerr << " (ARRAY_BOUNDS_EXCEEDED)"; break;
        case EXCEPTION_STACK_OVERFLOW: std::cerr << " (STACK_OVERFLOW)"; break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO: std::cerr << " (INT_DIVIDE_BY_ZERO)"; break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO: std::cerr << " (FLT_DIVIDE_BY_ZERO)"; break;
        default: break;
        }
        std::cerr << "\n";

        // Capture stack trace
        if (pExceptionInfo->ContextRecord)
        {
            std::string stackTrace = CaptureStackTrace(pExceptionInfo->ContextRecord);
            std::cerr << stackTrace;
        }
    }

    std::cerr << "[CRITICAL] Input file: " << g_InputPath.string() << "\n";
    std::cerr << "[CRITICAL] ========================================\n";
    std::cerr.flush();

    CompilationResult crashResult;
    crashResult.success = false;

    // Try to save some info before dying
    WriteManifest(crashResult, true);

    return EXCEPTION_EXECUTE_HANDLER; // Pass to OS to kill process
}


TextureCompressionFormat ParseCompressionFormat(const std::string& fmt)
{
    std::string upper = fmt;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);

    if (upper == "BC1") return TextureCompressionFormat::BC1;
    if (upper == "BC3") return TextureCompressionFormat::BC3;
    if (upper == "BC4") return TextureCompressionFormat::BC4;
    if (upper == "BC5") return TextureCompressionFormat::BC5;
    if (upper == "BC7") return TextureCompressionFormat::BC7;
    if (upper == "ASTC") return TextureCompressionFormat::ASTC;
    if (upper == "ETC") return TextureCompressionFormat::ETC;

    return TextureCompressionFormat::UNCOMPRESSED;
}

void SetupCLIArguments(Parser& parser)
{
    // General
    parser.AddOption("root", "Path to Assets root directory (in magicengine, use fs::path(Filepaths::assets))", false, 1);
    parser.AddOption("input", "Path to source asset (fbx, png, etc.) or compiled asset for thumbnail-only mode", true, 1);
    parser.AddOption("output", "Directory to output compiled assets", true, 1);
    parser.AddOption("platform", "Target Platform (windows/android)", false, 1);

    // Thumbnail-only mode (for generating thumbnails of existing compiled assets)
    parser.AddOption("thumbnail-only", "Generate thumbnail only, input is a compiled asset (ktx2, material)", false, 0);

    // Recompression mode (for producing platform variants from existing compiled KTX2)
    parser.AddOption("recompress-ktx2", "Recompress an existing compiled .ktx2 into the target platform format (e.g. BC7 -> ASTC for Android)", false, 0);

    // Mesh Options - but if you dont wanna optimise it, then what's the point of doing this even?
    parser.AddOption("no-opt", "Disable mesh optimization", false, 0);
    parser.AddOption("no-tangents", "Skip tangent generation", false, 0);
    parser.AddOption("noflip-uv", "Don't flip UV coordinates", false, 0);

    // Texture Options
    parser.AddOption("format", "Compression format (BC1, BC3, BC7, ASTC, ETC)", false, 1);
    parser.AddOption("mipcount", "Mipmap generation count", false, 1);
    parser.AddOption("quality", "Compression quality (0.0 - 1.0)", false, 1);
}


int main(int argc, char* argv[])
{
    g_StartTime = GetCurrentTimeStr();
    SetUnhandledExceptionFilter(UnhandledException);

    Parser parser;
    SetupCLIArguments(parser);

    if (!parser.Parse(argc, argv))
    {
        parser.PrintHelp();
        return 1; // Error code
    }

    // Check for special modes first (thumbnail-only doesn't need --root)
    const bool thumbnailOnly = parser.HasOption("thumbnail-only");
    if (thumbnailOnly)
    {
        std::string inputRaw = parser.GetOption<std::string>("input").value();
        std::string outputRaw = parser.GetOption<std::string>("output").value();

        std::filesystem::path inputAbs = std::filesystem::absolute(inputRaw).lexically_normal();
        std::filesystem::path outputAbs = std::filesystem::absolute(outputRaw).lexically_normal();

        // Write manifest location
        g_ManifestPath = std::filesystem::path(argv[0]).parent_path() / "CompileResult.json";
        g_InputPath = inputAbs;

        std::cerr << "[AssetCompiler] Thumbnail-only mode\n";
        std::cerr << "[AssetCompiler] Input: " << inputAbs.string() << "\n";
        std::cerr << "[AssetCompiler] Output: " << outputAbs.string() << "\n";
        std::cerr.flush();

        ProgressReporter::ReportProgress(0.0f, "Generating thumbnail", inputAbs.filename().string());

        CompilationResult thumbResult;
        thumbResult.success = false;

        std::string ext = inputAbs.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        try
        {
            if (ext == ".ktx2")
            {
                std::cerr << "[AssetCompiler] Processing KTX2 texture...\n";
                std::cerr.flush();

                TextureCompiler textureCompiler;
                thumbResult.success = textureCompiler.GenerateThumbnailOnly(inputAbs, outputAbs);
                if (!thumbResult.success)
                {
                    std::cerr << "[AssetCompiler] GenerateThumbnailOnly returned false\n";
                    thumbResult.errors.push_back("Failed to generate thumbnail for: " + inputAbs.string());
                }
                else
                {
                    std::cerr << "[AssetCompiler] Thumbnail generated successfully\n";
                }
            }
            else if (ext == ".material")
            {
                // DISABLED: Material thumbnail generation needs debugging
                std::cerr << "[AssetCompiler] Material thumbnail generation temporarily disabled\n";
                thumbResult.errors.push_back("Material thumbnail generation temporarily disabled");
            }
            else if (ext == ".mesh")
            {
                // DISABLED: Mesh thumbnail generation needs debugging
                std::cerr << "[AssetCompiler] Mesh thumbnail generation temporarily disabled\n";
                thumbResult.errors.push_back("Mesh thumbnail generation temporarily disabled");
            }
            else
            {
                std::cerr << "[ERROR] Unsupported file type for thumbnail-only mode: " << ext << std::endl;
                thumbResult.errors.push_back("Unsupported file type for thumbnail-only: " + ext);
            }
        }
        catch (const std::exception& e)
        {
            std::cerr << "[AssetCompiler] EXCEPTION: " << e.what() << "\n";
            std::cerr << CaptureStackTrace();
            std::cerr.flush();
            thumbResult.success = false;
            thumbResult.errors.push_back(std::string("Exception: ") + e.what());
        }
        catch (...)
        {
            std::cerr << "[AssetCompiler] UNKNOWN EXCEPTION!\n";
            std::cerr << CaptureStackTrace();
            std::cerr.flush();
            thumbResult.success = false;
            thumbResult.errors.push_back("Unknown exception caught");
        }

        std::cerr << "[AssetCompiler] Writing manifest and exiting with code: " << (thumbResult.success ? 0 : 1) << "\n";
        std::cerr.flush();

        WriteManifest(thumbResult);
        ProgressReporter::ReportComplete(thumbResult.success,
            thumbResult.success ? "Thumbnail generated" : "Thumbnail generation failed");
        return thumbResult.success ? 0 : 1;
    }

    // Recompress compiled KTX2 to a platform-specific GPU format.
    // This is intended for generating Android ASTC variants from existing Windows BCn compiled assets.
    const bool recompressKtx2 = parser.HasOption("recompress-ktx2");

#pragma region Parsing Options
    // Regular compilation mode - requires --root
    std::string rootRaw = parser.GetOption<std::string>("root").value_or("");
    if (rootRaw.empty())
    {
        std::cerr << "[ERROR] --root is required for regular compilation mode\n";
        parser.PrintHelp();
        return 1;
    }

    std::string inputRaw = parser.GetOption<std::string>("input").value();
    std::string outputRaw = parser.GetOption<std::string>("output").value();

    std::filesystem::path rootAbs = std::filesystem::absolute(rootRaw).lexically_normal();
    std::filesystem::path inputAbs = std::filesystem::absolute(inputRaw).lexically_normal();
    std::filesystem::path outputAbs = std::filesystem::absolute(outputRaw).lexically_normal();


    CompilerOptions options;

    // General options
    options.general.assetsRoot = rootAbs;
    options.general.inputPath = inputAbs;
    options.general.outputPath = outputAbs;

    options.mesh.optimize = !parser.HasOption("no-opt");
    options.mesh.generateTangents = !parser.HasOption("no-tangents");
    options.mesh.flipUVs = !parser.HasOption("noflip-uv"); //We flip by default anyway

    // Texture Defaults
    if (auto count = parser.GetOption<int>("mipcount"))
    {
        if (count.value() > 1)
        {
            options.texture.generateMipmaps = true;
        }
        options.texture.mipCount = count.value();
    }

    // Quality
    if (auto q = parser.GetOption<double>("quality"))
    {
        options.texture.quality = static_cast<float>(q.value());
    }


    if (auto fmt = parser.GetOption<std::string>("format"))
    {
        // User override
        options.texture.compressionFormat = ParseCompressionFormat(fmt.value());
    }

    std::string platform = parser.GetOption<std::string>("platform").value_or("windows");
    bool isAndroid = (platform == "android");

    // Set platform in options for metadata tracking
    options.general.platform = isAndroid ? BUILD_PLATFORM::ANDROID : BUILD_PLATFORM::WINDOWS;

    if (auto fmt = parser.GetOption<std::string>("format"))
    {
        options.texture.compressionFormat = ParseCompressionFormat(fmt.value());
    }
    else
    {
        // Defaults BC7 for windows and ASTC for android if not specified
        // ASTC is supported by all modern Android GPUs (Mali, Adreno, etc.)
        if (isAndroid)
            options.texture.compressionFormat = TextureCompressionFormat::ASTC;
        else
            options.texture.compressionFormat = TextureCompressionFormat::BC7;
    }
#pragma endregion

    // Time to Compile
    std::filesystem::path path = options.general.inputPath;
    if (!std::filesystem::exists(path))
    {
        std::cerr << "[ERROR] Input file does not exist: " << path << std::endl;
        ProgressReporter::ReportComplete(false, "Input file does not exist");
        return 1;
    }
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


    // Write manifest to where this .exe is located
    g_ManifestPath = std::filesystem::path(argv[0]).parent_path() / "CompileResult.json";
    g_InputPath = options.general.inputPath;

    // Report starting compilation
    ProgressReporter::ReportProgress(0.0f, "Starting compilation", path.filename().string());

    CompilationResult finalResult;
    finalResult.success = false;

    if (recompressKtx2)
    {
        // Input should be a compiled .ktx2
        if (ext != ".ktx2")
        {
            std::cerr << "[ERROR] --recompress-ktx2 requires a .ktx2 input. Got: " << ext << std::endl;
            ProgressReporter::ReportComplete(false, "Invalid input for recompress mode");
            return 1;
        }

        ProgressReporter::ReportProgress(0.1f, "Recompressing KTX2", path.filename().string());
        TextureCompiler textureCompiler;
        finalResult = textureCompiler.RecompressKTX2(options);
        WriteManifest(finalResult);

        WriteManifest(finalResult, false);
        ProgressReporter::ReportComplete(finalResult.success,
            finalResult.success ? "Recompress successful" : "Recompress failed");
        return finalResult.success ? 0 : 1;
    }

    if (ext == ".fbx" || ext == ".glb")
    {
        ProgressReporter::ReportProgress(0.1f, "Loading scene", path.filename().string());
        SceneCompiler sceneCompiler;
        finalResult = sceneCompiler.Compile(options);
        WriteManifest(finalResult);
    }
    else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
    {
        ProgressReporter::ReportProgress(0.1f, "Compressing texture", path.filename().string());
        TextureCompiler textureCompiler;
        finalResult = textureCompiler.Compile(options);
        WriteManifest(finalResult);
    }
    else
    {
        std::cerr << "[ERROR] Unsupported file extension: " << ext << std::endl;
        ProgressReporter::ReportComplete(false, "Unsupported file extension");
        return 1;
    }

    WriteManifest(finalResult, false);

    // Report completion
    ProgressReporter::ReportComplete(finalResult.success,
        finalResult.success ? "Compilation successful" : "Compilation failed");

    return finalResult.success ? 0 : 1;
}
