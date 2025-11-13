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
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "Engine/Resources/ResourceFilepaths.h"
#include "Engine/Resources/ResourceNames.h"

class MagicResourceManager
{
public:
    void Init();
    void Shutdown();

    static UserResourceGetter<ResourceMesh> Meshes();
    static UserResourceGetter<ResourceMaterial> Materials();
    static UserResourceGetter<ResourceTexture> Textures();
    static UserResourceGetter<ResourceAnimation> Animations();
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
    const ResourceContainerAnimations& Editor_GetAnimations();
    const ResourceContainerAudio& Editor_GetAudio();
    const std::string& Editor_GetName(size_t hash);

public:
    ResourceFilepaths& INTERNAL_GetFilepathsManager();
    ResourceNames& INTERNAL_GetNamesManager();

    ResourceContainerMeshes& INTERNAL_GetMeshes();
    ResourceContainerMaterials& INTERNAL_GetMaterials();
    ResourceContainerTextures& INTERNAL_GetTextures();
    ResourceContainerAnimations& INTERNAL_GetAnimations();
    ResourceContainerAudio& INTERNAL_GetAudio();

    void INTERNAL_CreateEmptyResource(size_t resourceTypeHash, size_t resourceHash);

private:
    ResourceFilepaths filepathsManager;
    ResourceNames namesManager;

    ResourceContainerMeshes meshes;
    ResourceContainerMaterials materials;
    ResourceContainerTextures textures;
    ResourceContainerAnimations animations;
    ResourceContainerAudio audio;

};
