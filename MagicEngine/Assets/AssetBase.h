/******************************************************************************/
/*!
\file   ResourceTypes.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides the base resource type and a container to store resources of user
defined resource types.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <unordered_map>
#include <unordered_set>
#include <memory>

struct ResourceBase
{
    size_t hash;

    virtual bool IsLoaded() = 0;
};

class IResourceContainer
{
public:
    virtual ~IResourceContainer() = default;

    virtual const void* GetResource(size_t hash) = 0;
    virtual void DeleteResource(size_t hash) = 0;

    virtual void INTERNAL_CreateResource(size_t hash) = 0;
};

template <std::derived_from<ResourceBase> ResourceType>
class ResourceContainerBase : public IResourceContainer
{
public:
    const void* GetResource(size_t hash) override;
    void DeleteResource(size_t hash) override;

public:
    void INTERNAL_CreateResource(size_t hash) override;
    ResourceType* INTERNAL_GetResource(size_t hash, bool createIfMissing);

    // Reference counting (tracked per-hash)
    void IncrementRef(size_t hash) { ++refCounts[hash]; }
    void DecrementRef(size_t hash) {
        auto it = refCounts.find(hash);
        if (it != refCounts.end() && --it->second == 0)
            refCounts.erase(it);
    }
    uint32_t GetRefCount(size_t hash) const {
        auto it = refCounts.find(hash);
        return it != refCounts.end() ? it->second : 0;
    }

private:
    std::unordered_map<size_t, std::unique_ptr<ResourceType>> resources;
    std::unordered_map<size_t, uint32_t> refCounts;
    std::unordered_set<size_t> m_deferredRequested;  // suppress repeat broadcasts

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

#include "AssetBase.ipp"
