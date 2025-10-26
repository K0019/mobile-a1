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

#include "Managers/AudioManager.h"
#include "FilepathConstants.h"

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "ryEngine"
#endif

void MagicResourceManager::Init()
{
    Messaging::Subscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Subscribe("ResourceDeleted", OnResourceDeleted);
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

UserResourceGetter<ResourceMesh> MagicResourceManager::Meshes()
{
    return UserResourceGetter<ResourceMesh>{ &ST<MagicResourceManager>::Get()->meshes };
}
UserResourceGetter<ResourceMaterial> MagicResourceManager::Materials()
{
    return UserResourceGetter<ResourceMaterial>{ &ST<MagicResourceManager>::Get()->materials };
}
UserResourceGetter<ResourceTexture> MagicResourceManager::Textures()
{
    return UserResourceGetter<ResourceTexture>{ &ST<MagicResourceManager>::Get()->textures };
}
UserResourceGetter<ResourceAudio> MagicResourceManager::Audio()
{
    return UserResourceGetter<ResourceAudio>{ &ST<MagicResourceManager>::Get()->audio };
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

const ResourceContainerMeshes& MagicResourceManager::Editor_GetMeshes()
{
    return meshes;
}
const ResourceContainerMaterials& MagicResourceManager::Editor_GetMaterials()
{
    return materials;
}
const ResourceContainerTextures& MagicResourceManager::Editor_GetTextures()
{
    return textures;
}
const ResourceContainerAudio& MagicResourceManager::Editor_GetAudio()
{
    return audio;
}

const std::string& MagicResourceManager::Editor_GetName(size_t hash)
{
    const std::string* name{ namesManager.GetName(hash) };
    assert(name);
    return *name;
}

ResourceFilepaths& MagicResourceManager::INTERNAL_GetFilepathsManager()
{
    return filepathsManager;
}
ResourceNames& MagicResourceManager::INTERNAL_GetNamesManager()
{
    return namesManager;
}
ResourceContainerMeshes& MagicResourceManager::INTERNAL_GetMeshes()
{
    return meshes;
}
ResourceContainerMaterials& MagicResourceManager::INTERNAL_GetMaterials()
{
    return materials;
}
ResourceContainerTextures& MagicResourceManager::INTERNAL_GetTextures()
{
    return textures;
}
ResourceContainerAudio& MagicResourceManager::INTERNAL_GetAudio()
{
    return audio;
}

void MagicResourceManager::INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash)
{
#ifndef GLFW
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "resourceTypeHash:               %llu", (unsigned long long)resourceTypeHash);

    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Androids ResourceMesh hash:     %llu", (unsigned long long)typeid(ResourceMesh).hash_code());
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Androids ResourceMaterial hash: %llu", (unsigned long long)typeid(ResourceMaterial).hash_code());
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Androids ResourceTexture hash:  %llu", (unsigned long long)typeid(ResourceTexture).hash_code());
    __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Androids ResourceAudio hash:    %llu", (unsigned long long)typeid(ResourceAudio).hash_code());
#endif

    // Sorry for this if spam kinda running out of time, this function existing's also kinda ugly anyway
    if (resourceTypeHash == util::ConsistentHash<ResourceMesh>())
        meshes.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == util::ConsistentHash<ResourceMaterial>())
        materials.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == util::ConsistentHash<ResourceTexture>())
        textures.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == util::ConsistentHash<ResourceAudio>())
        audio.INTERNAL_CreateResource(resourceHash);
    else
        assert(false);
}
