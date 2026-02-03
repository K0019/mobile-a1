/******************************************************************************/
/*!
\file   ResourceManager.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Central interface to access registered assets such as meshes, and audio.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Assets/AssetBase.h"
#include "Assets/AssetFilepaths.h"
#include "Assets/AssetNames.h"
#include <deque>
#include <unordered_set>

class AssetManager
{
public:
    void Init();
    void Shutdown();

    // Load an asset immediately (synchronous). Looks up filepath by hash, imports the file.
    void LoadAssetImmediate(size_t hash);

    // Queue an asset for deferred loading. Will be processed by ProcessPendingLoads().
    void QueueDeferredLoad(size_t hash);

    // Process pending resource loads, bounded by both count and time budget.
    // Always processes at least 1 to guarantee forward progress.
    void ProcessPendingLoads(int maxCount = 4, float budgetMs = 8.0f);

    template <typename ResourceType>
    static UserResourceGetter<ResourceType> GetContainer();

    void SaveToFile() const;
    void LoadFromFile();

private:
    // Messaging callbacks (forward to public methods)
    static void OnResourceRequestedLoad(size_t hash);
    static void OnResourceLoadDeferred(size_t hash);
    static void OnResourceDeleted(size_t hash, size_t resourceType);

public:
    template <typename ResourceType>
    const ResourceContainerBase<ResourceType>& Editor_GetContainer();
    const std::string* Editor_GetName(size_t hash);

    // Get reference count for a specific asset (for editor debugging)
    template <typename ResourceType>
    uint32_t Editor_GetRefCount(size_t hash);


public:
    AssetFilepaths& INTERNAL_GetFilepathsManager();
    AssetNames& INTERNAL_GetNamesManager();

    template <typename ResourceType>
    ResourceContainerBase<ResourceType>& INTERNAL_GetContainer();

    void INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash);

private:
    AssetFilepaths filepathsManager;
    AssetNames namesManager;

    std::unordered_map<size_t, UPtr<IResourceContainer>> resourceContainers;

    // Deferred resource loading queue — hashes are queued by GetResource()
    // and processed in batches each frame by ProcessPendingLoads()
    std::deque<size_t> m_pendingLoadQueue;
    std::unordered_set<size_t> m_pendingLoadSet;  // dedup

};

template<typename ResourceType>
UserResourceGetter<ResourceType> AssetManager::GetContainer()
{
    return UserResourceGetter<ResourceType>{ &ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceType>() };
}

template<typename ResourceType>
const ResourceContainerBase<ResourceType>& AssetManager::Editor_GetContainer()
{
    return INTERNAL_GetContainer<ResourceType>();
}

template<typename ResourceType>
ResourceContainerBase<ResourceType>& AssetManager::INTERNAL_GetContainer()
{
    static const size_t hash = util::ConsistentHash<ResourceType>();  // Cache hash per type
    auto containerIter{ resourceContainers.find(hash) };
    if (containerIter != resourceContainers.end())
        return static_cast<ResourceContainerBase<ResourceType>&>(*containerIter->second);
    return static_cast<ResourceContainerBase<ResourceType>&>(*resourceContainers.try_emplace(hash, std::make_unique<ResourceContainerBase<ResourceType>>()).first->second);
}

template<typename ResourceType>
uint32_t AssetManager::Editor_GetRefCount(size_t hash)
{
    return INTERNAL_GetContainer<ResourceType>().GetRefCount(hash);
}
