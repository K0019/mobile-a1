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

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include <string>
#include <unordered_map>

class ResourceImporter
{
public:
    // Function pointer type for import functions
    using ImportFn = bool(*)(const std::string&);

    static bool Import(const std::string& filepath);
    static bool FiletypeSupported(const std::string& extension);

private:
    static std::unordered_map<std::string, ImportFn> importers;

};
