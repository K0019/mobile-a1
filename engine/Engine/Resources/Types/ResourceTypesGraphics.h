#pragma once
#include "ResourceTypes.h"
#include "asset_types.h"

struct ResourceMesh : public ResourceBase
{
    MeshHandle handle;
    Mat4 transform;

    virtual bool IsLoaded() final;
};
using ResourceContainerMeshes = ResourceContainerBase<ResourceMesh>;

struct ResourceMaterial : public ResourceBase
{
    MaterialHandle handle;

    virtual bool IsLoaded() final;
};
using ResourceContainerMaterials = ResourceContainerBase<ResourceMaterial>;
