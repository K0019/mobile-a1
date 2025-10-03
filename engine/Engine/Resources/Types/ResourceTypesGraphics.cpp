#include "ResourceTypesGraphics.h"

bool ResourceMesh::IsLoaded()
{
    return !handles.empty() && handles[0].isValid();
}

bool ResourceMaterial::IsLoaded()
{
    return handle.isValid();
}

bool ResourceTexture::IsLoaded()
{
    return handle.isValid();
}