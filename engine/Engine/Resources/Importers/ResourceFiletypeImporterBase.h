#pragma once
#include "ResourceFilepaths.h"
#include "ResourceManager.h"

class ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) = 0;

protected:
    static size_t GenerateNewHash();
    static std::filesystem::path GetExeRelativeFilepath(const std::filesystem::path& assetRelativeFilepath);

    template <typename ...ResourceTypes, typename ...ResourceCountType>
    static const ResourceFilepaths::FileEntry* GenerateFileEntryForResources(const std::filesystem::path& assetRelativeFilepath, ResourceCountType... numResources);
private:
    template <typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
    static void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType... remaining);
    template <typename ...RemainingResourceTypes>
    static void GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes);
protected:
    template <typename ResourceType>
    static void GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes);
    static void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::filesystem::path& relativeFilepath);

private:

};

template<typename ...ResourceTypes, typename ...ResourceCountType>
const ResourceFilepaths::FileEntry* ResourceFiletypeImporterBase::GenerateFileEntryForResources(const std::filesystem::path& assetRelativeFilepath, ResourceCountType ...numResources)
{
    if (const ResourceFilepaths::FileEntry* existingFileEntry{ ST<ResourceManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(assetRelativeFilepath)})
        return existingFileEntry;

    std::vector<AssociatedResourceHashes> resourceHashes{ sizeof...(ResourceTypes) };
    GenerateHashForOneResourceType<ResourceTypes...>(resourceHashes.begin(), numResources...);
    GenerateNamesForResources(resourceHashes, assetRelativeFilepath);
    return ST<ResourceManager>::Get()->INTERNAL_GetFilepathsManager().SetFilepath(assetRelativeFilepath, std::move(resourceHashes));
}

template<typename ResourceType, typename ...RemainingResourceTypes, typename ...ResourceCountType>
void ResourceFiletypeImporterBase::GenerateHashForOneResourceType(std::vector<AssociatedResourceHashes>::iterator resourceHashes, size_t numResource, ResourceCountType ...remaining)
{
    GenerateHashesForResourceType<ResourceType>(resourceHashes._Ptr, numResource);
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

