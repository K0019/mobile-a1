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

#include "Engine/Resources/ResourceImporter.h"
#include "Engine/Resources/Types/ResourceTypes.h"
#include "Engine/Resources/ResourceManager.h"
#include "FilepathConstants.h"

#include "Engine/Resources/Importers/ResourceFiletypeImporterFBX.h"
#include "Engine/Resources/Importers/ResourceFiletypeImporterKTX.h"
#include "Engine/Resources/Importers/ResourceFiletypeImporterMeshAsset.h"
#include "Engine/Resources/Importers/ResourceFiletypeImporterMaterial.h"
#include "Engine/Resources/Importers/ResourceFiletypeImporterImage.h"
#include "Engine/Resources/Importers/ResourceFiletypeImporterAudio.h"

std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> ResourceImporter::importers{
    { std::string{ ".fbx" }, std::make_shared<ResourceFiletypeImporterFBX>() },
    { std::string{ ".ktx" }, std::make_shared<ResourceFiletypeImporterKTX>() },
    { std::string{ ".ktx2" }, std::make_shared<ResourceFiletypeImporterKTX>() },
    { std::string{ ".mesh" }, std::make_shared<ResourceFiletypeImporterMeshAsset>() },
    { std::string{ ".material" }, std::make_shared<ResourceFiletypeImporterMaterial>() },
    { std::string{ ".png" }, std::make_shared<ResourceFiletypeImporterImage>() },
    { std::string{ ".jpg" }, std::make_shared<ResourceFiletypeImporterImage>() },
    { std::string{ ".jpeg" }, std::make_shared<ResourceFiletypeImporterImage>() },
    { std::string{ ".bmp" }, std::make_shared<ResourceFiletypeImporterImage>() },
    { std::string{ ".mp3" }, std::make_shared<ResourceFiletypeImporterAudio>() },
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
    auto relativeFilepath{ std::filesystem::relative(filepath, Filepaths::assets) };

    return filetypeImporterIter->second->Import(relativeFilepath);
}

bool ResourceImporter::FiletypeSupported(const std::string& extension)
{
    return importers.find(extension) != importers.end();
}
