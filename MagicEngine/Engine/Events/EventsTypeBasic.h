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

namespace Events {

	// Consumed by: GrabbableItemPickupUISystem
	struct SceneUnloaded
	{
		int index;
	};

	// Consumed by: LuaScripting
	struct RequestReloadLuaScripts {};
	// Consumed by: ScriptRefreshListenerSystem
	struct LuaScriptsReloaded {};

	// Consumed by: PlayerCharacter
	struct GameActionGrabItem {};
	struct GameActionThrowItem {};
	struct GameActionAttack {};
	struct GameActionDodge {};

}

namespace Getters {

	// Provided by: CustomViewport
	struct MousePosViewport {};

}
