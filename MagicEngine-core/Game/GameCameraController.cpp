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
#include "Engine/Input.h"

GameCameraControllerComponent::GameCameraControllerComponent()
	: cameraEntity{ nullptr }
	, playerEntity{ nullptr }
	, cameraPitch{ 0.0f }
	, cameraYaw{ 0.0f }
	, cameraSensitivity{ 1.0f }
{
}

//GameCameraControllerComponent::~GameCameraControllerComponent()
//{
//}

void GameCameraControllerComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("cameraEntity", cameraEntity);
	writer.Serialize("playerEntity", playerEntity);
	//.Serialize(writer);
	//playerEntity.Serialize(writer);
}

void GameCameraControllerComponent::Deserialize(Deserializer& reader)
{
	reader.Deserialize("cameraEntity", &cameraEntity);
	reader.Deserialize("playerEntity", &playerEntity);
	//cameraEntity.Deserialize(reader);
	//playerEntity.Deserialize(reader);
}

void GameCameraControllerComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
	cameraEntity.EditorDraw("Camera");
	playerEntity.EditorDraw("Player");
	gui::TextBoxReadOnly("Yaw", std::to_string(cameraYaw));
	gui::TextBoxReadOnly("Pitch", std::to_string(cameraPitch));
#endif
}

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	if (!comp.playerEntity || !comp.cameraEntity)
		return;

	Vec3 offset;

	offset.x = 0.0f;
	offset.y = 10.0f;
	offset.z = 10.0f;

	Vec3 playerPos = comp.playerEntity->GetTransform().GetWorldPosition();
	Vec3 cameraPos = playerPos + offset;

	comp.cameraEntity->GetTransform().SetWorldPosition(cameraPos);

	float camYaw = 0.0f;
	float camPitch = 45.0f;

	comp.cameraEntity->GetTransform().SetWorldRotation({ camPitch, camYaw, 0.0f });
}
