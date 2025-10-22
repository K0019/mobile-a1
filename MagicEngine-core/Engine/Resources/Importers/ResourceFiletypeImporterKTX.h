/******************************************************************************/
/*!
\file   ResourceFiletypeImporterKTX.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/27/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for KTX files.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"

namespace Resource {
    struct ProcessedTexture;
}

class ResourceFiletypeImporterKTX : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::string& assetRelativeFilepath) final;

#ifndef GLFW
private:
    Resource::ProcessedTexture ManualLoadTexture(const std::string& filepath);
#endif

};
