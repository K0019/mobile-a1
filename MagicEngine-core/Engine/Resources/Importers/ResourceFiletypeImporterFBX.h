/******************************************************************************/
/*!
\file   ResourceFiletypeImporterFBX.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
A resource importer for FBX files.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Engine/Resources/Importers/ResourceFiletypeImporterBase.h"

class ResourceFiletypeImporterFBX : public ResourceFiletypeImporterBase
{
public:
    virtual bool Import(const std::filesystem::path& assetRelativeFilepath) final;

};

