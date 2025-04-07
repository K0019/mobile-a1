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
#include "GameCameraController.h"
#include "TweenManager.h"
#include "Messaging.h"

GameCameraControllerComponent::GameCameraControllerComponent() :
#ifdef IMGUI_ENABLED
	REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
	cameraEntity{ nullptr },
	playerEntity{ nullptr },
	minX{ 0.0f },
	maxX{ 0.0f },
	minY{ 0.0f },
	maxY{ 0.0f },
	offsetAmount{ 0.0f },
	offsetDuration{ 1.0f },
	offsetAmountCurrent{ 0.0f }
{
}

GameCameraControllerComponent::~GameCameraControllerComponent()
{
}

void GameCameraControllerComponent::SetOffsetCurrent(float offset)
{
	offsetAmountCurrent = offset;
}

#ifdef IMGUI_ENABLED
void GameCameraControllerComponent::EditorDraw(GameCameraControllerComponent& comp)
{
	comp.cameraEntity.EditorDraw("Camera");
	comp.playerEntity.EditorDraw("Player");
	ImGui::InputFloat("Min X", &comp.minX);
	ImGui::InputFloat("Max X", &comp.maxX);
	ImGui::InputFloat("Min Y", &comp.minY);
	ImGui::InputFloat("Max Y", &comp.maxY);
	ImGui::InputFloat("Offset Amount", &comp.offsetAmount);
	ImGui::InputFloat("Offset Duration", &comp.offsetDuration);
}
#endif

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}

void GameCameraControllerSystem::OnAdded()
{
	Messaging::Subscribe("WaveStarted", GameCameraControllerSystem::OnWaveStarted);
}

void GameCameraControllerSystem::OnRemoved()
{
	Messaging::Unsubscribe("WaveStarted", GameCameraControllerSystem::OnWaveStarted);
}

void GameCameraControllerSystem::OnWaveStarted()
{
	// Get the camera controller component
	auto it = ecs::GetCompsActiveBegin<GameCameraControllerComponent>();
	ecs::CompHandle<GameCameraControllerComponent> comp = it.GetComp();
	ecs::EntityHandle gameCamera = it.GetEntity();

	// If cannot find game camera, just return.
	if (!gameCamera) return;

	// Tween camera
	ST<TweenManager>::Get()->StartTween(
		gameCamera,
		&GameCameraControllerComponent::SetOffsetCurrent,
		0.0f,
		comp->offsetAmount,
		comp->offsetDuration,
		TT::EASE_BOTH
	);
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	// If no player or camera reference, just return.
	if (!comp.playerEntity || !comp.cameraEntity)
	{
		return;
	}

	// Find player position
	Vector2 playerPosition = comp.playerEntity->GetTransform().GetWorldPosition();
	float x = math::Clamp(playerPosition.x, comp.minX, comp.maxX);
	float y = math::Clamp(playerPosition.y + comp.offsetAmountCurrent, comp.minY, comp.maxY);
	comp.cameraEntity->GetTransform().SetWorldPosition(Vector2(x, y));
}
