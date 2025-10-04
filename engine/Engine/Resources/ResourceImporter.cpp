/******************************************************************************/
/*!
\file   ResourceImporter.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides importers for supported file types.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ResourceImporter.h"
#include "ResourceTypes.h"
#include "ResourceManager.h"

#include "ResourceFiletypeImporterFBX.h"
#include "ResourceFiletypeImporterKTX.h"
#include "ResourceFiletypeImporterAudio.h"

#include "GameSettings.h"

std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> ResourceImporter::importers{
    { std::string{ ".fbx" }, std::make_shared<ResourceFiletypeImporterFBX>() },
    { std::string{ ".ktx" }, std::make_shared<ResourceFiletypeImporterKTX>() },
    { std::string{ ".mp3" }, std::make_shared<ResourceFiletypeImporterAudio>() }
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

bool ResourceImporter::FiletypeSupported(const std::string& extension)
{
    return importers.find(extension) != importers.end();
}
