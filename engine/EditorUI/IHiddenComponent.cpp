/******************************************************************************/
/*!
\file   IHiddenComponent.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing IHiddenComponent, which hides ecs components from
  being displayed in the editor inspector window.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "IHiddenComponent.h"

bool HiddenComponentsStore::IsHidden(ecs::CompHash compHash)
{
	return ST<HiddenComponentsStore>::Get()->registeredCompHashes.find(compHash) != ST<HiddenComponentsStore>::Get()->registeredCompHashes.end();
}

void HiddenComponentsStore::RegisterComponent(ecs::CompHash compHash)
{
	ST<HiddenComponentsStore>::Get()->registeredCompHashes.insert(compHash);
}
