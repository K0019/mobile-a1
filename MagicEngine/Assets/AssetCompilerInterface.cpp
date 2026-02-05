#include "FilepathConstants.h"
#include "AssetFilepaths.h"
#include "Assets/AssetImporter.h"
#include "AssetCompilerInterface.h"
#include "Utilities/ProcessRunner.h"
#include "VFS/VFS.h"
#include "Utilities/Serializer.h"

std::string ReadFileToString(const std::filesystem::path path)
{
    std::ifstream file(path);
    if (!file.is_open())
        return std::string{};

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool CompileAndImportAsset(const std::string& assetRelativeFilepath)
{
    if (!std::filesystem::exists(Filepaths::compilerExe))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Compiler not found: " << Filepaths::compilerExe;
        return false;
    }

    std::filesystem::path fileToCompile = std::filesystem::absolute(VFS::ConvertVirtualToPhysical(assetRelativeFilepath));
    std::filesystem::path outputPath = std::filesystem::absolute(VFS::ConvertVirtualToPhysical("/CompiledAssets"));

    // Construct Command Line
    std::string cmd = "\"" + Filepaths::compilerExe + "\"";
    cmd += " --input \"" + fileToCompile.string() + "\"";
    cmd += " --output \"" + outputPath.string() + "\"";
    cmd += " --root \"" + Filepaths::assets + "\"";

    ProcessResult result = RunProcess(cmd);

    CONSOLE_LOG(LEVEL_INFO) << "----- Asset Compiler Logs START -----";

    if (result.exitCode != 0)
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Asset Compilation failed ";
        CONSOLE_LOG(LEVEL_ERROR) << result.output;
    }
    else
    {
        CONSOLE_LOG(LEVEL_INFO) << "Asset Compilation succeeded ";
        CONSOLE_LOG(LEVEL_INFO) << result.output;
    }

    CONSOLE_LOG(LEVEL_INFO) << "----- Asset Compiler Logs END   -----";

    // Compiler hardcodes result information in same directory as the exe
    if (!std::filesystem::exists(Filepaths::compileManifest))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Compile finished, but CompileResult.json is missing! Skipping importing.";
        return false;
    }
    
    // We read contents instead of passing filepath to dodge the VFS inside deserializer. VFS works in assets directory only because we don't mount anyth else.
    std::string manifestContents = ReadFileToString(Filepaths::compileManifest);
    Deserializer deserializer { manifestContents }; 
    if (!deserializer.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize";
    }
    std::vector<std::string> meshes;
    std::vector<std::string> textures;
    std::vector<std::string> materials;
    std::vector<std::string> animations;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    deserializer.DeserializeVar("meshes", &meshes);
    deserializer.DeserializeVar("textures", &textures);
    deserializer.DeserializeVar("materials", &materials);
    deserializer.DeserializeVar("animations", &animations);
    deserializer.DeserializeVar("errors", &errors);
    deserializer.DeserializeVar("warnings", &warnings);

    if (!warnings.empty())
    {
        CONSOLE_LOG(LEVEL_WARNING) << "----- Compile Warnings START -----";
        for (const auto& warning : warnings)
        {
            CONSOLE_LOG(LEVEL_WARNING) << warning;
        }
        CONSOLE_LOG(LEVEL_WARNING) << "----- Compile Warnings END -----";
    }

    if (!errors.empty())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "----- Compile Errors START -----";
        for (const auto& error : errors)
        {
            CONSOLE_LOG(LEVEL_ERROR) << error;
        }
        CONSOLE_LOG(LEVEL_ERROR) << "Importing of assets aborted.";
        CONSOLE_LOG(LEVEL_ERROR) << "----- Compile Errors END -----";
        return false;
    }


    auto ImportList = [&](const std::vector<std::string>& compiledAssets) {
        for (const auto& path : compiledAssets)
        {
            // Skip thumbnail artifacts — they are not importable assets
            if (path.find("_thumb.") != std::string::npos)
                continue;

            std::filesystem::path absPath = path;
            std::string toImport = VFS::ConvertPhysicalToVirtual(absPath.string());
            AssetImporter::Import(toImport);
        }
    };

    ImportList(textures);
    ImportList(materials);
    ImportList(meshes);
    ImportList(animations);

    return true;
}