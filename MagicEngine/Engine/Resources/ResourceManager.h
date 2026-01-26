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
#include "Engine/Resources/Types/ResourceTypes.h"
#include "Engine/Resources/ResourceFilepaths.h"
#include "Engine/Resources/ResourceNames.h"

class MagicResourceManager
{
public:
    void Init();
    void Shutdown();

    template <typename ResourceType>
    static UserResourceGetter<ResourceType> GetContainer();

    void SaveToFile() const;
    void LoadFromFile();

private:
    static void OnResourceRequestedLoad(size_t hash);
    static void OnResourceDeleted(size_t hash, size_t resourceType);

public:
    template <typename ResourceType>
    const ResourceContainerBase<ResourceType>& Editor_GetContainer();
    const std::string* Editor_GetName(size_t hash);

public:
    ResourceFilepaths& INTERNAL_GetFilepathsManager();
    ResourceNames& INTERNAL_GetNamesManager();

    template <typename ResourceType>
    ResourceContainerBase<ResourceType>& INTERNAL_GetContainer();

    void INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash);

private:
    ResourceFilepaths filepathsManager;
    ResourceNames namesManager;

    std::unordered_map<size_t, UPtr<IResourceContainer>> resourceContainers;

};

template<typename ResourceType>
UserResourceGetter<ResourceType> MagicResourceManager::GetContainer()
{
    return UserResourceGetter<ResourceType>{ &ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceType>() };
}

template<typename ResourceType>
const ResourceContainerBase<ResourceType>& MagicResourceManager::Editor_GetContainer()
{
    return INTERNAL_GetContainer<ResourceType>();
}

template<typename ResourceType>
ResourceContainerBase<ResourceType>& MagicResourceManager::INTERNAL_GetContainer()
{
    static const size_t hash = util::ConsistentHash<ResourceType>();  // Cache hash per type
    auto containerIter{ resourceContainers.find(hash) };
    if (containerIter != resourceContainers.end())
        return static_cast<ResourceContainerBase<ResourceType>&>(*containerIter->second);
    return static_cast<ResourceContainerBase<ResourceType>&>(*resourceContainers.try_emplace(hash, std::make_unique<ResourceContainerBase<ResourceType>>()).first->second);
}
