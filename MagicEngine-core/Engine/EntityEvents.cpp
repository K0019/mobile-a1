/******************************************************************************/
/*!
\file   EntityLayers.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   01/15/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file for entity layers.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EntityEvents.h"

uint64_t EntityEventsComponent::GenerateIdentifier(ecs::EntityHandle entity, size_t functionHash)
{
	// TODO: [RANDOM] This could theoretically collide, no matter how small the chance is. Should try to find a solution to avoid this.
	return reinterpret_cast<uint64_t>(entity) & 0xFFFF'FFFF'0000'0000 + functionHash & 0xFFFF'FFFF;
}
