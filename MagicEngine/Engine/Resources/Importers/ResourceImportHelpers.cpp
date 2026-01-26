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
#include <filesystem>

namespace ResourceImportHelpers {

    size_t GenerateNewHash()
    {
        size_t hash{ util::Rand_UID() | 1 }; // Fill first bit so hash caching can use it to check for validity
        while (ST<MagicResourceManager>::Get()->INTERNAL_GetNamesManager().GetName(hash))
            hash = util::Rand_UID() | 1;
        CONSOLE_LOG(LEVEL_INFO) << "Generated new hash: " << hash;
        return hash;
    }

    size_t GetOrCreateResourceHash(const std::string& assetRelativeFilepath)
    {
        // Convert VFS path to physical path
        std::filesystem::path physicalPath = VFS::ConvertVirtualToPhysical(assetRelativeFilepath);
        std::filesystem::path metaPath = Resource::AssetMetadata::getMetaPath(physicalPath);

        // Try to load existing .meta file
        Resource::AssetMetadata meta;
        if (meta.loadFromFile(metaPath) && meta.hasResourceHash())
        {
            CONSOLE_LOG(LEVEL_INFO) << "Using existing hash from .meta: " << meta.resourceHash << " for " << assetRelativeFilepath;
            return meta.resourceHash;
        }

        // Generate new hash
        size_t newHash = GenerateNewHash();

        // Try to save it back to .meta file (update existing or create new)
        if (std::filesystem::exists(metaPath))
        {
            // Load existing meta to preserve other fields
            meta.loadFromFile(metaPath);
        }
        else
        {
            // Infer type from extension
            meta.assetType = Resource::AssetMetadata::inferFromPath(assetRelativeFilepath);
        }

        meta.resourceHash = newHash;

        if (meta.saveToFile(metaPath))
        {
            CONSOLE_LOG(LEVEL_INFO) << "Saved new hash to .meta: " << newHash << " for " << assetRelativeFilepath;
        }
        else
        {
            CONSOLE_LOG(LEVEL_WARNING) << "Failed to save hash to .meta for " << assetRelativeFilepath;
        }

        return newHash;
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
