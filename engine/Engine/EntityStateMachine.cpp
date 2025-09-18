/******************************************************************************/
/*!
\file   EntityStateMachine.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing a set of classes specialized for a state
  machine compatible with the ECS architecture.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "EntityStateMachine.h"

namespace sm {

	ECSStateMachine::ECSStateMachine(State* startingState)
		: StateMachine{ startingState }
		, entity{ nullptr }
	{
	}

	void ECSStateMachine::Update(ecs::EntityHandle thisEntity)
	{
		entity = thisEntity;
		StateMachine::Update();
	}

	ecs::EntityHandle ECSStateMachine::GetEntity() const
	{
		return entity;
	}

}
