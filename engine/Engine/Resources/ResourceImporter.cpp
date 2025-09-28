#include "ResourceImporter.h"
#include "ResourceTypes.h"
#include "ResourceManager.h"

#include "ResourceFiletypeImporterFBX.h"
#include "ResourceFiletypeImporterMeshAsset.h"

std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> ResourceImporter::importers{
    { std::string{ ".fbx" }, std::make_shared<ResourceFiletypeImporterFBX>() },
    { std::string{ ".mesh" }, std::make_shared<ResourceFiletypeImporterMeshAsset>() }
};

bool ResourceImporter::Import(const std::filesystem::path& filepath)
{
    // Check file exists
    if (!std::filesystem::exists(filepath))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File does not exist: " << filepath;
        return false;
    }

    // Get the importer for this filetype
    std::string filetype{ util::ToLowerStr(filepath.extension().string()) };
    auto filetypeImporterIter{ importers.find(filetype) };
    if (filetypeImporterIter == importers.end())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Filetype not supported: " << filetype;
        return false;
    }

    // Import the file, creating/updating the resources in ResourceManager
    auto relativeFilepath{ std::filesystem::relative(filepath, ST<Filepaths>::Get()->assets) };

    return filetypeImporterIter->second->Import(relativeFilepath);
}
