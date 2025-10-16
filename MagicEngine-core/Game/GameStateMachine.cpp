/******************************************************************************/
/*!
\file   GameStateMachine.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing a set of classes implementing state machine behaviors
  for use in game.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "GameStateMachine.h"

namespace sm {

	bool TransitionAlways::Decide(StateMachine*)
	{
		return true;
	}

	bool sm::TransitionTimed::Decide(StateMachine*)
	{
		timer -= GameTime::FixedDt();
		return timer <= 0.0f;
	}

}
