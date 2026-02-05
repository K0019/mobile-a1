/******************************************************************************/
/*!
\file   ResourceManager.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Central interface to access registered assets such as meshes, and audio.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Assets/AssetManager.h"
#include "Assets/AssetImporter.h"
#include "Assets/AssetSerialization.h"
#include <chrono>

#include "Assets/Types/AssetTypesAudio.h"
#include "Assets/Types/AssetTypesVideo.h"
#include "Assets/Types/AssetTypes.h"
#include "Game/Weapon.h"
#include "FilepathConstants.h"
#include "VFS/VFS.h"

void AssetManager::Init()
{
    Messaging::Subscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Subscribe("NeedResourceLoadDeferred", OnResourceLoadDeferred);
    Messaging::Subscribe("ResourceDeleted", OnResourceDeleted);

    // Unfortunately since we don't have automatic type information when we deserialize, we need to register our types manually..
    INTERNAL_GetContainer<ResourceAudio>();
    INTERNAL_GetContainer<ResourceAudioGroup>();
    INTERNAL_GetContainer<ResourceMesh>();
    INTERNAL_GetContainer<ResourceMaterial>();
    INTERNAL_GetContainer<ResourceTexture>();
    INTERNAL_GetContainer<ResourceAnimation>();
    INTERNAL_GetContainer<ResourceVideo>();
    INTERNAL_GetContainer<WeaponInfo>();
}
void AssetManager::Shutdown()
{
    Messaging::Unsubscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Unsubscribe("NeedResourceLoadDeferred", OnResourceLoadDeferred);
    Messaging::Unsubscribe("ResourceDeleted", OnResourceDeleted);
}

void AssetManager::LoadAssetImmediate(size_t resourceHash)
{
    if (const auto* fileEntry{ filepathsManager.GetFileEntry(resourceHash) })
        AssetImporter::Import(fileEntry->path);
}

void AssetManager::QueueDeferredLoad(size_t hash)
{
    // Deduplicate: only queue if not already pending
    if (m_pendingLoadSet.insert(hash).second)
    {
        m_pendingLoadQueue.push_back(hash);
    }
}

void AssetManager::OnResourceRequestedLoad(size_t hash)
{
    ST<AssetManager>::Get()->LoadAssetImmediate(hash);
}

void AssetManager::OnResourceLoadDeferred(size_t hash)
{
    ST<AssetManager>::Get()->QueueDeferredLoad(hash);
}

void AssetManager::ProcessPendingLoads(int maxCount, float budgetMs)
{
    if (m_pendingLoadQueue.empty()) return;

    auto start = std::chrono::steady_clock::now();
    int processed = 0;

    while (!m_pendingLoadQueue.empty() && processed < maxCount)
    {
        size_t hash = m_pendingLoadQueue.front();
        m_pendingLoadQueue.pop_front();
        m_pendingLoadSet.erase(hash);

        // Import the resource synchronously (on the main thread, but one at a time)
        if (const auto* fileEntry = filepathsManager.GetFileEntry(hash))
        {
            AssetImporter::Import(fileEntry->path);
        }
        ++processed;

        // Always allow at least 1 import for forward progress, then check time budget
        if (processed > 0)
        {
            auto elapsed = std::chrono::steady_clock::now() - start;
            float elapsedMs = std::chrono::duration<float, std::milli>(elapsed).count();
            if (elapsedMs >= budgetMs)
                break;
        }
    }
}

void AssetManager::OnResourceDeleted(size_t hash, size_t resourceType)
{
    ST<AssetManager>::Get()->filepathsManager.DisassociateResourceHash(hash, resourceType);
    ST<AssetManager>::Get()->namesManager.RemoveName(hash);
}

void AssetManager::SaveToFile() const
{
    Serializer writer{ VFS::ConvertVirtualToPhysical(Filepaths::assetsJson) };
    if (!writer.IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to save resources: " << Filepaths::assetsJson;
        return;
    }

    AssetSerialization::Serialize(writer, filepathsManager, namesManager);
}
void AssetManager::LoadFromFile()
{
    Deserializer reader{ Filepaths::assetsJson };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open resources: " << Filepaths::assetsJson;
        return;
    }

    AssetSerialization::Deserialize(reader, &filepathsManager, &namesManager, [](size_t resourceTypeHash, size_t resourceHash) -> void {
        ST<AssetManager>::Get()->INTERNAL_CreateEmptyResource(resourceTypeHash, resourceHash);
    });
}

const std::string* AssetManager::Editor_GetName(size_t hash)
{
    return namesManager.GetName(hash);
}

AssetFilepaths& AssetManager::INTERNAL_GetFilepathsManager()
{
    return filepathsManager;
}
AssetNames& AssetManager::INTERNAL_GetNamesManager()
{
    return namesManager;
}

void AssetManager::INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash)
{
    auto containerIter{ resourceContainers.find(resourceTypeHash) };
    if (containerIter == resourceContainers.end())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Resource container does not exist for hash " << resourceTypeHash << "! Did you forget to initialize a container for a new resource?";
        return;
    }
    containerIter->second->INTERNAL_CreateResource(resourceHash);
}
