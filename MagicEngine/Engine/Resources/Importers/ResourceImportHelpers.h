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
#include "Engine/Resources/ResourceFilepaths.h"
#include "Engine/Resources/ResourceManager.h"

namespace ResourceImportHelpers {

    size_t GenerateNewHash();

    std::filesystem::path GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath);

    void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::string& relativeFilepath);

    template <typename ResourceType>
    void GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes);

    template <typename ...ResourceTypes, typename ...ResourceCountType>
    const ResourceFilepaths::FileEntry* GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType... numResources);

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
    resource->resourceTypeHash = util::ConsistentHash<ResourceType>();
    for (size_t i{}; i < numHashes; ++i)
        resource->hashes.push_back(GenerateNewHash());
}

template<typename ...ResourceTypes, typename ...ResourceCountType>
const ResourceFilepaths::FileEntry* ResourceImportHelpers::GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType ...numResources)
{
    if (const ResourceFilepaths::FileEntry* existingFileEntry{ ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath)})
        return existingFileEntry;

    std::vector<AssociatedResourceHashes> resourceHashes{ sizeof...(ResourceTypes) };
    detail::GenerateHashForOneResourceType<ResourceTypes...>(resourceHashes.begin(), numResources...);
    GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
    return ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes));
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
