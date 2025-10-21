/******************************************************************************/
/*!
\file   ResourceNames.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/25/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
Manages the association of a name to each resource hash.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Engine/Resources/ResourceNames.h"

const std::string* ResourceNames::GetName(size_t hash) const
{
    auto nameIter{ names.find(hash) };
    return (nameIter != names.end() ? &nameIter->second : nullptr);
}

void ResourceNames::SetName(size_t hash, const std::string& name)
{
    names[hash] = name;
}

void ResourceNames::RemoveName(size_t hash)
{
    names.erase(hash);
}