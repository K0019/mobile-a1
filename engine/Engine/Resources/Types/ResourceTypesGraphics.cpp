#include "ResourceTypesGraphics.h"

bool ResourceMesh::IsLoaded()
{
    return handle.isValid();
}

bool ResourceMaterial::IsLoaded()
{
    return handle.isValid();
}