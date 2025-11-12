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

#include "Engine/Resources/ResourceManager.h"
#include "Engine/Resources/ResourceImporter.h"
#include "Engine/Resources/ResourceSerialization.h"

#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "FilepathConstants.h"

void MagicResourceManager::Init()
{
    Messaging::Subscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Subscribe("ResourceDeleted", OnResourceDeleted);

    // Unfortunately since we don't have automatic type information when we deserialize, we need to register our types manually..
    INTERNAL_GetContainer<ResourceAudio>();
    INTERNAL_GetContainer<ResourceMesh>();
    INTERNAL_GetContainer<ResourceMaterial>();
    INTERNAL_GetContainer<ResourceTexture>();
}
void MagicResourceManager::Shutdown()
{
    Messaging::Unsubscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Unsubscribe("ResourceDeleted", OnResourceDeleted);
}

void MagicResourceManager::OnResourceRequestedLoad(size_t resourceHash)
{
    if (const auto* fileEntry{ ST<MagicResourceManager>::Get()->filepathsManager.GetFileEntry(resourceHash) })
        //ResourceImporter::Import(Filepaths::assets / fileEntry->path);
        //ResourceImporter::Import(VFS::JoinPath("assets", fileEntry->path));
        ResourceImporter::Import(fileEntry->path);
}

void MagicResourceManager::OnResourceDeleted(size_t hash, size_t resourceType)
{
    ST<MagicResourceManager>::Get()->filepathsManager.DisassociateResourceHash(hash, resourceType);
    ST<MagicResourceManager>::Get()->namesManager.RemoveName(hash);
}

void MagicResourceManager::SaveToFile() const
{
    Serializer writer{ VFS::ConvertVirtualToPhysical(Filepaths::assetsJson) };
    if (!writer.IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to save resources: " << Filepaths::assetsJson;
        return;
    }

    ResourceSerialization::Serialize(writer, filepathsManager, namesManager);
}
void MagicResourceManager::LoadFromFile()
{
    Deserializer reader{ Filepaths::assetsJson };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open resources: " << Filepaths::assetsJson;
        return;
    }

    ResourceSerialization::Deserialize(reader, &filepathsManager, &namesManager, [](size_t resourceTypeHash, size_t resourceHash) -> void {
        ST<MagicResourceManager>::Get()->INTERNAL_CreateEmptyResource(resourceTypeHash, resourceHash);
    });
}

const std::string* MagicResourceManager::Editor_GetName(size_t hash)
{
    return namesManager.GetName(hash);
}

ResourceFilepaths& MagicResourceManager::INTERNAL_GetFilepathsManager()
{
    return filepathsManager;
}
ResourceNames& MagicResourceManager::INTERNAL_GetNamesManager()
{
    return namesManager;
}

void MagicResourceManager::INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash)
{
    auto containerIter{ resourceContainers.find(resourceTypeHash) };
    if (containerIter == resourceContainers.end())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Resource container does not exist for hash " << resourceTypeHash << "! Did you forget to initialize a container for a new resource?";
        return;
    }
    containerIter->second->INTERNAL_CreateResource(resourceHash);
}
