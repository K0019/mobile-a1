/******************************************************************************/
/*!
\file   ResourceManager.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Resource manager class that hosts several functions to load Textures and Shaders. Each loaded texture
and/or shader is also stored for future reference by string handles. All functions and resources are static and no
public constructor is defined.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ResourceManager.h"
#include "ResourceImporter.h"
#include "ResourceSerialization.h"

#include "AudioManager.h"

const std::string* ResourceNames::GetName(size_t hash) const
{
    auto nameIter{ names.find(hash) };
    return (nameIter != names.end() ? &nameIter->second : nullptr);
}

void ResourceNames::SetName(size_t hash, const std::string& name)
{
    names[hash] = name;
}

void ResourceNames::RemoveName(size_t hash)
{
    names.erase(hash);
}

void ResourceManager::Init()
{
    Messaging::Subscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Subscribe("ResourceDeleted", OnResourceDeleted);
}
void ResourceManager::Shutdown()
{
    Messaging::Unsubscribe("NeedResourceLoaded", OnResourceRequestedLoad);
    Messaging::Unsubscribe("ResourceDeleted", OnResourceDeleted);
}

void ResourceManager::OnResourceRequestedLoad(size_t resourceHash)
{
    if (const auto* fileEntry{ ST<ResourceManager>::Get()->filepathsManager.GetFileEntry(resourceHash) })
        ResourceImporter::Import(ST<Filepaths>::Get()->assets / fileEntry->path);
}

void ResourceManager::OnResourceDeleted(size_t hash, size_t resourceType)
{
    ST<ResourceManager>::Get()->filepathsManager.DisassociateResourceHash(hash, resourceType);
    ST<ResourceManager>::Get()->namesManager.RemoveName(hash);
}

UserResourceGetter<ResourceMesh> ResourceManager::Meshes()
{
    return UserResourceGetter<ResourceMesh>{ &ST<ResourceManager>::Get()->meshes };
}
UserResourceGetter<ResourceMaterial> ResourceManager::Materials()
{
    return UserResourceGetter<ResourceMaterial>{ &ST<ResourceManager>::Get()->materials };
}
UserResourceGetter<ResourceTexture> ResourceManager::Textures()
{
    return UserResourceGetter<ResourceTexture>{ &ST<ResourceManager>::Get()->textures };
}
UserResourceGetter<ResourceAudio> ResourceManager::Audio()
{
    return UserResourceGetter<ResourceAudio>{ &ST<ResourceManager>::Get()->audio };
}

void ResourceManager::SaveToFile() const
{
    Serializer writer{ ST<Filepaths>::Get()->assetsJson };
    if (!writer.IsOpen())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to save resources: " << ST<Filepaths>::Get()->assetsJson;
        return;
    }

    ResourceSerialization::Serialize(writer, filepathsManager, namesManager);
}
void ResourceManager::LoadFromFile()
{
    Deserializer reader{ ST<Filepaths>::Get()->assetsJson };
    if (!reader.IsValid())
    {
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to open resources: " << ST<Filepaths>::Get()->assetsJson;
        return;
    }

    ResourceSerialization::Deserialize(reader, &filepathsManager, &namesManager, [](size_t resourceTypeHash, size_t resourceHash) -> void {
        ST<ResourceManager>::Get()->INTERNAL_CreateEmptyResource(resourceTypeHash, resourceHash);
    });
}

const ResourceContainerMeshes& ResourceManager::Editor_GetMeshes()
{
    return meshes;
}
const ResourceContainerMaterials& ResourceManager::Editor_GetMaterials()
{
    return materials;
}
const ResourceContainerTextures& ResourceManager::Editor_GetTextures()
{
    return textures;
}
const ResourceContainerAudio& ResourceManager::Editor_GetAudio()
{
    return audio;
}

const std::string& ResourceManager::Editor_GetName(size_t hash)
{
    const std::string* name{ namesManager.GetName(hash) };
    assert(name);
    return *name;
}

ResourceFilepaths& ResourceManager::INTERNAL_GetFilepathsManager()
{
    return filepathsManager;
}
ResourceNames& ResourceManager::INTERNAL_GetNamesManager()
{
    return namesManager;
}
ResourceContainerMeshes& ResourceManager::INTERNAL_GetMeshes()
{
    return meshes;
}
ResourceContainerMaterials& ResourceManager::INTERNAL_GetMaterials()
{
    return materials;
}
ResourceContainerTextures& ResourceManager::INTERNAL_GetTextures()
{
    return textures;
}
ResourceContainerAudio& ResourceManager::INTERNAL_GetAudio()
{
    return audio;
}

void ResourceManager::INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash)
{
    // Sorry for this if spam kinda running out of time, this function existing's also kinda ugly anyway
    if (resourceTypeHash == typeid(ResourceMesh).hash_code())
        meshes.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == typeid(ResourceMaterial).hash_code())
        materials.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == typeid(ResourceTexture).hash_code())
        textures.INTERNAL_CreateResource(resourceHash);
    else if (resourceTypeHash == typeid(ResourceAudio).hash_code())
        audio.INTERNAL_CreateResource(resourceHash);
    else
        assert(false);
}
