/******************************************************************************/
/*!
\file   ResourceFiletypeImporterBase.h
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

#pragma once
#include "VFS/VFS.h"
#include "Engine/Resources/ResourceFilepaths.h"
#include "Engine/Resources/ResourceManager.h"

class ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::string& assetRelativeFilepath) = 0;

protected:
    static size_t GenerateNewHash();

    static std::filesystem::path GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath);

    template <typename ...ResourceTypes, typename ...ResourceCountType>
    static const ResourceFilepaths::FileEntry* GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType... numResources);

private:
    template <typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
    static void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType... remaining);
    template <typename ...RemainingResourceTypes>
    static void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes);
protected:
    template <typename ResourceType>
    static void GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes);
    static void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::string& relativeFilepath);

private:

};

template<typename ...ResourceTypes, typename ...ResourceCountType>
const ResourceFilepaths::FileEntry* ResourceFiletypeImporterBase::GenerateFileEntryForResources(const std::string& assetRelativeFilepath, ResourceCountType ...numResources)
{
    if (const ResourceFilepaths::FileEntry* existingFileEntry{ ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath)})
        return existingFileEntry;

    std::vector<AssociatedResourceHashes> resourceHashes{ sizeof...(ResourceTypes) };
    GenerateHashForOneResourceType<ResourceTypes...>(resourceHashes.begin(), numResources...);
    GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
    return ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes));
}

template<typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
void ResourceFiletypeImporterBase::GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType ...remaining)
{
    GenerateHashesForResourceType<ResourceType>(&*resourceHashes, numResource);
    GenerateHashForOneResourceType<RemainingResourceTypes...>(++resourceHashes, remaining...);
}
template <typename ...RemainingResourceTypes>
void ResourceFiletypeImporterBase::GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator)
{
}

template <typename ResourceType>
void ResourceFiletypeImporterBase::GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes)
{
    resource->resourceTypeHash = typeid(ResourceType).hash_code();
    for (size_t i{}; i < numHashes; ++i)
        resource->hashes.push_back(GenerateNewHash());
}

