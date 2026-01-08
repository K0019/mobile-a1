/******************************************************************************/
/*!
\file   LuaScripting.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   11/01/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides an interface for lua scripting.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

namespace luabridge {
	class Namespace;
}

void RegisterCppStuffToLua(luabridge::Namespace baseTable);
