/******************************************************************************/
/*!
\file   LuaScripting.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/21/2024

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e
\brief
  This file contains the definitions of the class Hot Reloader, which reloads
  the and updates the scripts inside the User Assembly .dll inside the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "LuaTypesECS.h"

LuaWrapperEntity::LuaWrapperEntity(ecs::EntityHandle entity)
	: entityRaw{ reinterpret_cast<size_t>(entity) }
{
}

ecs::EntityHandle LuaWrapperEntity::operator->()
{
	return GetHandle();
}

LuaWrapperEntity::operator ecs::EntityHandle()
{
	return GetHandle();
}

inline ecs::EntityHandle LuaWrapperEntity::GetHandle() const
{
	return reinterpret_cast<ecs::EntityHandle>(entityRaw);
}