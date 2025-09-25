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
