/******************************************************************************/
/*!
\file   ResourceImportHelpers.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5

\brief
Utility functions for resource importers. Extracted from ResourceFiletypeImporterBase
to eliminate unnecessary OOP inheritance pattern.

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "VFS/VFS.h"
#include "Assets/AssetFilepaths.h"
#include "Assets/AssetManager.h"
#include "resource/asset_metadata.h"
#include <tuple>

namespace ResourceImportHelpers {

    size_t GenerateNewHash();

    /**
     * @brief Get resource hash from .meta file or generate a new one.
     *
     * If a .meta file exists for the asset and has a resourceHash, returns it.
     * Otherwise generates a new hash and saves it to the .meta file.
     *
     * @param assetRelativeFilepath VFS path to the compiled asset
     * @return The resource hash (existing from .meta or newly generated)
     */
    size_t GetOrCreateResourceHash(const std::string& assetRelativeFilepath);

    std::filesystem::path GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath);

    void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::string& relativeFilepath);

    template <typename ResourceType>
    void GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes);

    template <typename ...ResourceTypes, typename ...ResourceCountType>
    const AssetFilepaths::FileEntry* GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType... numResources);

    // Implementation details
    namespace detail {
        template <typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
        void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType... remaining);

        template <typename ...RemainingResourceTypes>
        void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes);
    }

} // namespace ResourceImportHelpers

// Template implementations

template <typename ResourceType>
void ResourceImportHelpers::GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes)
{
    static const size_t typeHash = util::ConsistentHash<ResourceType>();  // Cache hash per type
    resource->resourceTypeHash = typeHash;
    for (size_t i{}; i < numHashes; ++i)
        resource->hashes.push_back(GenerateNewHash());
}

template<typename ...ResourceTypes, typename ...ResourceCountType>
const AssetFilepaths::FileEntry* ResourceImportHelpers::GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType ...numResources)
{
    if (const AssetFilepaths::FileEntry* existingFileEntry{ ST<AssetManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath)})
        return existingFileEntry;

    // For single-resource files (common case: .mesh, .ktx2, .material, .anim),
    // try to use hash from .meta file to preserve identity across reimports
    constexpr size_t numResourceTypes = sizeof...(ResourceTypes);
    const size_t totalResources = (static_cast<size_t>(numResources) + ...);

    std::vector<AssociatedResourceHashes> resourceHashes{ numResourceTypes };

    if (numResourceTypes == 1 && totalResources == 1)
    {
        // Single resource case: use GetOrCreateResourceHash for .meta file support
        using FirstResourceType = std::tuple_element_t<0, std::tuple<ResourceTypes...>>;
        static const size_t typeHash = util::ConsistentHash<FirstResourceType>();
        resourceHashes[0].resourceTypeHash = typeHash;
        resourceHashes[0].hashes.push_back(GetOrCreateResourceHash(assetRelativeFilepath));
    }
    else
    {
        // Multiple resources: fall back to random hash generation
        detail::GenerateHashForOneResourceType<ResourceTypes...>(resourceHashes.begin(), numResources...);
    }

    GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
    return ST<AssetManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes));
}

template<typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
void ResourceImportHelpers::detail::GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType ...remaining)
{
    GenerateHashesForResourceType<ResourceType>(&*resourceHashes, numResource);
    GenerateHashForOneResourceType<RemainingResourceTypes...>(++resourceHashes, remaining...);
}

template <typename ...RemainingResourceTypes>
void ResourceImportHelpers::detail::GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator)
{
}
