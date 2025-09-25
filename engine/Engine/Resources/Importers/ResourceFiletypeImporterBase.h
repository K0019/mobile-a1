#pragma once
#include "ResourceFilepaths.h"

class ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& relativeFilepath) = 0;

protected:
    static size_t GenerateNewHash();

    template <typename ResourceType>
    static void GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes);
    static void GenerateNamesForResources(std::vector<AssociatedResourceHashes>& resources, const std::filesystem::path& relativeFilepath);

private:

};

template <typename ResourceType>
void ResourceFiletypeImporterBase::GenerateHashesForResourceType(AssociatedResourceHashes* resource, size_t numHashes)
{
    resource->resourceTypeHash = typeid(ResourceType).hash_code();
    for (size_t i{}; i < numHashes; ++i)
        resource->hashes.push_back(GenerateNewHash());
}

