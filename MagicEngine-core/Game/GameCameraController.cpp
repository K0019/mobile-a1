/******************************************************************************/
/*!
\file   GameCameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	GameCameraController is an ECS component-system pair which takes control of
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/GameCameraController.h"
#include "Tween/TweenManager.h"
#include "Utilities/Messaging.h"
#include "Editor/Containers/GUICollection.h"

GameCameraControllerComponent::GameCameraControllerComponent()
	: cameraEntity{ nullptr }
	, playerEntity{ nullptr }
	, minX{ 0.0f }
	, maxX{ 0.0f }
	, minY{ 0.0f }
	, maxY{ 0.0f }
	, offsetAmount{ 0.0f }
	, offsetDuration{ 1.0f }
	, offsetAmountCurrent{ 0.0f }
{
}

GameCameraControllerComponent::~GameCameraControllerComponent()
{
}

void GameCameraControllerComponent::SetOffsetCurrent(float offset)
{
	offsetAmountCurrent = offset;
}

void GameCameraControllerComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
	cameraEntity.EditorDraw("Camera");
	playerEntity.EditorDraw("Player");
#endif
}

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	// If no player or camera reference, just return.
	if (!comp.playerEntity || !comp.cameraEntity)
		return;

	// Find player position

}
