/******************************************************************************/
/*!
\file   ECSSysLayers.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/24/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This source file implements standardized ECS system layers.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ECSSysLayers.h"

namespace ecs {

	void RunSystemsInLayers(ECS_LAYER prevCutoff, ECS_LAYER untilOffset)
	{
		for (int index{ static_cast<int>(prevCutoff) + 1 }; index < static_cast<int>(untilOffset); ++index)
			ecs::RunSystems(static_cast<ECS_LAYER>(index));
	}

}
