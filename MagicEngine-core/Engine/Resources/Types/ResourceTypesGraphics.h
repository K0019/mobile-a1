/******************************************************************************/
/*!
\file   ResourceTypesGraphics.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Defines resources for graphics.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ResourceTypes.h"
#include "asset_types.h"

struct ResourceMesh : public ResourceBase
{
    std::vector<MeshHandle> handles;
    std::vector<Mat4> transforms;

    virtual bool IsLoaded() final;
};
using ResourceContainerMeshes = ResourceContainerBase<ResourceMesh>;

struct ResourceMaterial : public ResourceBase
{
    MaterialHandle handle;

    virtual bool IsLoaded() final;
};
using ResourceContainerMaterials = ResourceContainerBase<ResourceMaterial>;

struct ResourceTexture : public ResourceBase
{
    TextureHandle handle;

    virtual bool IsLoaded() final;
};
using ResourceContainerTextures = ResourceContainerBase<ResourceTexture>;
