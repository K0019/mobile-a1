/******************************************************************************/
/*!
\file   ResourceImporter.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Provides importers for supported file types.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ResourceFiletypeImporterBase.h"

class ResourceImporter
{
public:
    static bool Import(const std::filesystem::path& filepath);
    static bool FiletypeSupported(const std::string& extension);

private:
    static std::unordered_map<std::string, SPtr<ResourceFiletypeImporterBase>> importers;

};

