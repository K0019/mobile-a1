/******************************************************************************/
/*!
\file   ResourceImporter.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides importers for supported file types.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "VFS/VFS.h"

#include "Engine/Resources/ResourceImporter.h"
#include "Engine/Resources/Importers/ResourceImporters.h"

std::unordered_map<std::string, ResourceImporter::ImportFn> ResourceImporter::importers{
    { std::string{ ".fbx" },      &ResourceImporters::ImportFBX },
    { std::string{ ".glb" },      &ResourceImporters::ImportFBX },
    { std::string{ ".ktx" },      &ResourceImporters::ImportKTX },
    { std::string{ ".ktx2" },     &ResourceImporters::ImportKTX },
    { std::string{ ".mesh" },     &ResourceImporters::ImportMeshAsset },
    { std::string{ ".material" }, &ResourceImporters::ImportMaterial },
    { std::string{ ".anim" },     &ResourceImporters::ImportAnimationAsset },
    { std::string{ ".png" },      &ResourceImporters::ImportImage },
    { std::string{ ".jpg" },      &ResourceImporters::ImportImage },
    { std::string{ ".jpeg" },     &ResourceImporters::ImportImage },
    { std::string{ ".bmp" },      &ResourceImporters::ImportImage },
    { std::string{ ".mp3" },      &ResourceImporters::ImportAudio },
    { std::string{ ".wav" },      &ResourceImporters::ImportAudio },
	{ std::string{".weapon"},    &ResourceImporters::ImportGameWeapon },
};

bool ResourceImporter::Import(const std::string& filepath)
{
    // Check file exists
    if (!VFS::FileExists(filepath))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File does not exist: " << filepath;
        return false;
    }

    // Get the importer for this filetype
    std::string filetype{ util::ToLowerStr(VFS::GetExtension(filepath)) };
    auto filetypeImporterIter{ importers.find(filetype) };
    if (filetypeImporterIter == importers.end())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Filetype not supported: " << filetype;
        return false;
    }

    // Import the file via the function pointer
    return filetypeImporterIter->second(filepath);
}

bool ResourceImporter::FiletypeSupported(const std::string& extension)
{
    return importers.find(extension) != importers.end();
}
