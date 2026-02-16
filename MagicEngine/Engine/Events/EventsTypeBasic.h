/******************************************************************************/
/*!
\file   EventsQueue.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "MagicMath.h"

namespace Events {

	struct TransitionScene
	{
		std::string toScenePath;
	};

	// Consumed by: GrabbableItemPickupUISystem
	struct SceneUnloaded
	{
		int index;
	};

	// Consumed by: LuaScripting
	struct RequestReloadLuaScripts {};
	// Consumed by: ScriptRefreshListenerSystem
	struct LuaScriptsReloaded {};

	struct GyroRotation
	{
		Vec3 rotation;
	};

	// Consumed by: NiceThrow
	struct Game_NiceThrow {};

}

namespace Getters {

	// Provided by: CustomViewport
	struct MousePosViewport {};

}
