#pragma once

struct ResourceBase
{
    size_t hash;

    virtual bool IsLoaded() = 0;
};

template <std::derived_from<ResourceBase> ResourceType>
class ResourceContainerBase
{
public:
    const ResourceType* GetResource(size_t hash);
    void DeleteResource(size_t hash);

public:
    void INTERNAL_CreateResource(size_t hash);
    ResourceType* INTERNAL_GetResource(size_t hash, bool createIfMissing);

private:
    std::unordered_map<size_t, ResourceType> resources;

public:
    auto Editor_GetAllResources() const;
};

template <std::derived_from<ResourceBase> ResourceType>
class UserResourceGetter
{
public:
    UserResourceGetter(ResourceContainerBase<ResourceType>* container);

    const ResourceType* GetResource(size_t hash) const;

private:
    ResourceContainerBase<ResourceType>* container;

};

#include "ResourceTypes.ipp"
