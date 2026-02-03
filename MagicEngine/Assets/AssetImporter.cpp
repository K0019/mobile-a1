/******************************************************************************/
/*!
\file   AssetImporter.cpp
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

#include "Assets/AssetImporter.h"
#include "Assets/Importers/AssetImporters.h"

#include <unordered_set>

std::unordered_map<std::string, AssetImporter::ImportFn> AssetImporter::importers{
    { std::string{ ".fbx" },      &AssetImporters::ImportFBX },
    { std::string{ ".glb" },      &AssetImporters::ImportFBX },
    { std::string{ ".ktx" },      &AssetImporters::ImportKTX },
    { std::string{ ".ktx2" },     &AssetImporters::ImportKTX },
    { std::string{ ".mesh" },     &AssetImporters::ImportMeshAsset },
    { std::string{ ".material" }, &AssetImporters::ImportMaterial },
    { std::string{ ".anim" },     &AssetImporters::ImportAnimationAsset },
    { std::string{ ".png" },      &AssetImporters::ImportImage },
    { std::string{ ".jpg" },      &AssetImporters::ImportImage },
    { std::string{ ".jpeg" },     &AssetImporters::ImportImage },
    { std::string{ ".bmp" },      &AssetImporters::ImportImage },
    { std::string{ ".mp3" },      &AssetImporters::ImportAudio },
    { std::string{ ".wav" },      &AssetImporters::ImportAudio },
    { std::string{ ".sg" },       &AssetImporters::ImportAudioGroup },   //load
	{ std::string{".weapon"},    &AssetImporters::ImportGameWeapon },
};

bool AssetImporter::Import(const std::string& filepath)
{
    std::string resolvedPath = filepath;
    std::string filetype{ util::ToLowerStr(VFS::GetExtension(filepath)) };

#ifdef __ANDROID__
    // On Android, raw images (PNG, JPG, etc.) are pre-compiled to KTX2 ASTC format.
    // Check for the compiled version first.
    static const std::unordered_set<std::string> rawImageExtensions = {
        ".png", ".jpg", ".jpeg", ".bmp"
    };
    if (rawImageExtensions.count(filetype))
    {
        // Try .ktx2 version: replace extension
        std::string ktx2Path = filepath.substr(0, filepath.rfind('.')) + ".ktx2";
        if (VFS::FileExists(ktx2Path))
        {
            resolvedPath = ktx2Path;
            filetype = ".ktx2";
        }
    }
#endif

    // Check file exists
    if (!VFS::FileExists(resolvedPath))
    {
        CONSOLE_LOG(LEVEL_ERROR) << "File does not exist: " << resolvedPath;
        return false;
    }

    // Get the importer for this filetype
    auto filetypeImporterIter{ importers.find(filetype) };
    if (filetypeImporterIter == importers.end())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Filetype not supported: " << filetype;
        return false;
    }

    // Import the file via the function pointer
    return filetypeImporterIter->second(resolvedPath);
}

bool AssetImporter::FiletypeSupported(const std::string& extension)
{
    return importers.find(extension) != importers.end();
}
