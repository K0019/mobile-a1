/******************************************************************************/
/*!
\file   ResourceImportHelpers.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
Utility functions for resource importers.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/Importers/ResourceImportHelpers.h"
#include "Engine/Resources/ResourceManager.h"
#include "FilepathConstants.h"

namespace ResourceImportHelpers {

    size_t GenerateNewHash()
    {
        size_t hash{ util::Rand_UID() | 1 }; // Fill first bit so hash caching can use it to check for validity
        while (ST<MagicResourceManager>::Get()->INTERNAL_GetNamesManager().GetName(hash))
            hash = util::Rand_UID() | 1;
        CONSOLE_LOG(LEVEL_INFO) << "Generated " << hash;
        return hash;
    }

    std::filesystem::path GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath)
    {
        return Filepaths::assets / assetRelativeFilepath;
    }

    void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::string& relativeFilepath)
    {
        auto& namesManager{ ST<MagicResourceManager>::Get()->INTERNAL_GetNamesManager() };
        std::string filename{ VFS::GetStem(relativeFilepath)};

        for (const auto& resource : resources)
            for (size_t i{}; i < resource.hashes.size(); ++i)
                namesManager.SetName(resource.hashes[i], filename + std::to_string(i));
    }

} // namespace ResourceImportHelpers
