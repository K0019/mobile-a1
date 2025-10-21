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

#pragma once

class ResourceNames
{
public:
    const std::string* GetName(size_t hash) const;
    void SetName(size_t hash, const std::string& name);
    void RemoveName(size_t hash);

private:
    std::unordered_map<size_t, std::string> names;

};
