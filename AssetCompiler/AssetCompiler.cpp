#include "CompileOptions.h"
#include "SceneCompiler.h"
#include "TextureCompiler.h"
#include "CmdlineParser.h"

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/prettywriter.h>

#include <fstream>
#include <chrono>
#include <ctime>
#include <windows.h>


using namespace compiler;


std::filesystem::path g_ManifestPath;
std::filesystem::path g_InputPath;
std::string g_StartTime;

// ----- Helpers ----- //
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

LONG WINAPI UnhandledException([[maybe_unused]] PEXCEPTION_POINTERS pExceptionInfo)
{
    std::cerr << "[CRITICAL] Unhandled Exception caught! Writing panic manifest...\n";

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
    parser.AddOption("root", "Path to Assets root directory (in magicengine, use fs::path(Filepaths::assets))", true, 1);
    parser.AddOption("input", "Path to source asset (fbx, png, etc.)", true, 1);
    parser.AddOption("output", "Directory to output compiled assets", true, 1);
    parser.AddOption("platform", "Target Platform (windows/android)", false, 1);

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

#pragma region Parsing Options
    std::string rootRaw = parser.GetOption<std::string>("root").value();
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

    if (auto fmt = parser.GetOption<std::string>("format"))
    {
        options.texture.compressionFormat = ParseCompressionFormat(fmt.value());
    }
    else
    {
        // Defaults BC7 for windows and ETC for android if not specified
        if (isAndroid)
            options.texture.compressionFormat = TextureCompressionFormat::ETC;
        else
            options.texture.compressionFormat = TextureCompressionFormat::BC7;
    }
#pragma endregion


    // Time to Compile
    std::filesystem::path path = options.general.inputPath;
    if (!std::filesystem::exists(path))
    {
        std::cerr << "[ERROR] Input file does not exist: " << path << std::endl;
        return 1;
    }
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);


    // Write manifest to where this .exe is located
    g_ManifestPath = std::filesystem::path(argv[0]).parent_path() / "CompileResult.json";
    g_InputPath = options.general.inputPath;

    CompilationResult finalResult;
    finalResult.success = false;

    if (ext == ".fbx" || ext == ".glb")
    {
        SceneCompiler sceneCompiler;
        finalResult = sceneCompiler.Compile(options);
        WriteManifest(finalResult);
    }
    else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".bmp")
    {
        TextureCompiler textureCompiler;
        finalResult = textureCompiler.Compile(options);
        WriteManifest(finalResult);
    }
    else
    {
        std::cerr << "[ERROR] Unsupported file extension: " << ext << std::endl;
        return 1;
    }

    WriteManifest(finalResult, false);
    return finalResult.success ? 0 : 1;
}