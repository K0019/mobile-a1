/******************************************************************************/
/*!
\file   ResourceFiletypeImporterBase.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides a base class for resource importers to inherit from, and provides
helper functions to make the process of creating a new resouce entry easier.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"
#include "Engine/Resources/ResourceManager.h"
#include "FilepathConstants.h"

size_t ResourceFiletypeImporterBase::GenerateNewHash()
{
    size_t hash{ util::Rand_UID() };
    while (ST<MagicResourceManager>::Get()->INTERNAL_GetNamesManager().GetName(hash))
        hash = util::Rand_UID();
    CONSOLE_LOG(LEVEL_INFO) << "Generated " << hash;
    return hash;
}

std::filesystem::path ResourceFiletypeImporterBase::GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath)
{
    return Filepaths::assets / assetRelativeFilepath;
}

void ResourceFiletypeImporterBase::GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::filesystem::path& relativeFilepath)
{
    auto& namesManager{ ST<MagicResourceManager>::Get()->INTERNAL_GetNamesManager() };
    std::string filename{ relativeFilepath.stem().string() };

    for (const auto& resource : resources)
        for (size_t i{}; i < resource.hashes.size(); ++i)
            namesManager.SetName(resource.hashes[i], filename + std::to_string(i));
}
