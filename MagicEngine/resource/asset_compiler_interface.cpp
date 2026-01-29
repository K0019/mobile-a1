// resource/asset_compiler_interface.cpp
#include "asset_compiler_interface.h"
#include "asset_loader.h"
#include "resource_manager.h"
#include "VFS/VFS.h"
#include "utilities/process_runner.h"
#include "logging/log.h"

#include <rapidjson/document.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Resource
{

// Static member initialization
std::filesystem::path AssetCompilerInterface::s_compilerPath;
std::filesystem::path AssetCompilerInterface::s_outputDirectory;
std::filesystem::path AssetCompilerInterface::s_assetsRoot;

void AssetCompilerInterface::SetCompilerPath(const std::filesystem::path& exePath)
{
    s_compilerPath = exePath;
}

void AssetCompilerInterface::SetOutputDirectory(const std::filesystem::path& outputDir)
{
    s_outputDirectory = outputDir;
}

void AssetCompilerInterface::SetAssetsRoot(const std::filesystem::path& assetsRoot)
{
    s_assetsRoot = assetsRoot;
}

CompileResult AssetCompilerInterface::CompileAsset(const std::string& vfsPath)
{
    CompileResult result;
    result.success = false;

    // Validate compiler path
    if (s_compilerPath.empty() || !std::filesystem::exists(s_compilerPath))
    {
        result.errors.push_back("AssetCompiler not found: " + s_compilerPath.string());
        LOG_ERROR("AssetCompiler not found: {}", s_compilerPath.string());
        return result;
    }

    // Convert VFS path to physical path
    std::filesystem::path inputPath = std::filesystem::absolute(VFS::ConvertVirtualToPhysical(vfsPath));

    if (!std::filesystem::exists(inputPath))
    {
        result.errors.push_back("Input file not found: " + inputPath.string());
        LOG_ERROR("Input file not found: {}", inputPath.string());
        return result;
    }

    // Determine output directory
    std::filesystem::path outputPath = s_outputDirectory;
    if (outputPath.empty())
    {
        // Default: CompiledAssets directory next to assets
        outputPath = s_assetsRoot.parent_path() / "CompiledAssets";
    }
    outputPath = std::filesystem::absolute(outputPath);

    // Ensure output directory exists
    std::filesystem::create_directories(outputPath);

    // Determine assets root
    std::filesystem::path assetsRoot = s_assetsRoot;
    if (assetsRoot.empty())
    {
        // Default to "assets" as the VFS root since VFS doesn't have GetAssetsRoot()
        assetsRoot = std::filesystem::absolute("assets");
    }

    // Build command line
    std::string cmd = "\"" + s_compilerPath.string() + "\"";
    cmd += " --input \"" + inputPath.string() + "\"";
    cmd += " --output \"" + outputPath.string() + "\"";
    cmd += " --root \"" + assetsRoot.string() + "\"";

    LOG_INFO("Running AssetCompiler: {}", cmd);

    // Run the compiler
    util::ProcessResult procResult = util::RunProcess(cmd);

    LOG_INFO("----- AssetCompiler Output START -----");
    if (procResult.exitCode != 0)
    {
        LOG_ERROR("Asset compilation failed (exit code {})", procResult.exitCode);
        LOG_ERROR("{}", procResult.output);
    }
    else
    {
        LOG_INFO("Asset compilation succeeded");
        LOG_DEBUG("{}", procResult.output);
    }
    LOG_INFO("----- AssetCompiler Output END -----");

    // Parse the manifest file (written by compiler next to the exe)
    std::filesystem::path manifestPath = s_compilerPath.parent_path() / "CompileResult.json";
    if (!std::filesystem::exists(manifestPath))
    {
        result.errors.push_back("CompileResult.json not found after compilation");
        LOG_ERROR("CompileResult.json not found at: {}", manifestPath.string());
        return result;
    }

    result = ParseManifest(manifestPath);

    // Log warnings and errors from manifest
    for (const auto& warning : result.warnings)
    {
        LOG_WARNING("Compiler: {}", warning);
    }
    for (const auto& error : result.errors)
    {
        LOG_ERROR("Compiler: {}", error);
    }

    return result;
}

bool AssetCompilerInterface::IsCompilableExtension(const std::string& extension)
{
    std::string ext = extension;
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    static const std::vector<std::string> compilable = {
        ".fbx", ".glb", ".gltf",           // 3D models
        ".png", ".jpg", ".jpeg", ".bmp"    // Images
    };

    for (const auto& e : compilable)
    {
        if (ext == e)
            return true;
    }
    return false;
}

std::vector<std::string> AssetCompilerInterface::GetCompilableExtensions()
{
    return { ".fbx", ".glb", ".gltf", ".png", ".jpg", ".jpeg", ".bmp" };
}

CompileResult AssetCompilerInterface::ParseManifest(const std::filesystem::path& manifestPath)
{
    CompileResult result;
    result.success = false;

    // Read manifest file
    std::ifstream file(manifestPath);
    if (!file.is_open())
    {
        result.errors.push_back("Failed to open manifest: " + manifestPath.string());
        return result;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string jsonStr = buffer.str();

    // Parse JSON
    rapidjson::Document doc;
    doc.Parse(jsonStr.c_str());

    if (doc.HasParseError() || !doc.IsObject())
    {
        result.errors.push_back("Failed to parse manifest JSON");
        return result;
    }

    // Check status
    if (doc.HasMember("status") && doc["status"].IsString())
    {
        std::string status = doc["status"].GetString();
        result.success = (status == "SUCCESS");
    }

    // Parse file arrays
    auto parsePathArray = [&doc](const char* key, std::vector<std::filesystem::path>& out) {
        if (doc.HasMember(key) && doc[key].IsArray())
        {
            for (const auto& item : doc[key].GetArray())
            {
                if (item.IsString())
                {
                    out.emplace_back(item.GetString());
                }
            }
        }
    };

    parsePathArray("meshes", result.meshFiles);
    parsePathArray("textures", result.textureFiles);
    parsePathArray("materials", result.materialFiles);
    parsePathArray("animations", result.animationFiles);

    // Parse errors and warnings
    auto parseStringArray = [&doc](const char* key, std::vector<std::string>& out) {
        if (doc.HasMember(key) && doc[key].IsArray())
        {
            for (const auto& item : doc[key].GetArray())
            {
                if (item.IsString())
                {
                    out.emplace_back(item.GetString());
                }
            }
        }
    };

    parseStringArray("errors", result.errors);
    parseStringArray("warnings", result.warnings);

    return result;
}

} // namespace Resource
