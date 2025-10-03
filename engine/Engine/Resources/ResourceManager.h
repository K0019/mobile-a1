/******************************************************************************/
/*!
\file   ResourceManager.h
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

#pragma once
#include "ResourceTypesGraphics.h"
#include "ResourceTypesAudio.h"
#include "ResourceFilepaths.h"
#include "ResourceNames.h"

#include "scene.h"

#include "AudioManager.h"

#include "AssetBrowser.h"
#include "Sprite.h"
#include "GameSettings.h"

class ResourceManager
{
public:
    void Init();
    void Shutdown();

    static UserResourceGetter<ResourceMesh> Meshes();
    static UserResourceGetter<ResourceMaterial> Materials();
    static UserResourceGetter<ResourceTexture> Textures();
    static UserResourceGetter<ResourceAudio> Audio();

    void SaveToFile() const;
    void LoadFromFile();

private:
    static void OnResourceRequestedLoad(size_t hash);
    static void OnResourceDeleted(size_t hash, size_t resourceType);

public:
    const ResourceContainerMeshes& Editor_GetMeshes();
    const ResourceContainerMaterials& Editor_GetMaterials();
    const ResourceContainerTextures& Editor_GetTextures();
    const ResourceContainerAudio& Editor_GetAudio();
    const std::string& Editor_GetName(size_t hash);

public:
    ResourceFilepaths& INTERNAL_GetFilepathsManager();
    ResourceNames& INTERNAL_GetNamesManager();

    ResourceContainerMeshes& INTERNAL_GetMeshes();
    ResourceContainerMaterials& INTERNAL_GetMaterials();
    ResourceContainerTextures& INTERNAL_GetTextures();
    ResourceContainerAudio& INTERNAL_GetAudio();

    void INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash);

private:
    ResourceFilepaths filepathsManager;
    ResourceNames namesManager;

    ResourceContainerMeshes meshes;
    ResourceContainerMaterials materials;
    ResourceContainerTextures textures;
    ResourceContainerAudio audio;

};
