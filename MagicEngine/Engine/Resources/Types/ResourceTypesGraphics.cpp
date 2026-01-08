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

#include "Engine/Resources/Types/ResourceTypesGraphics.h"

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

bool ResourceAnimation::IsLoaded()
{
    return handle != Resource::INVALID_CLIP_ID;
}
