/******************************************************************************/
/*!
\file   GameCameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

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
	, currentCameraDistance{ 20.0f }
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
	cameraEntity.EditorDraw("Camera");
	playerEntity.EditorDraw("Player");
	gui::TextBoxReadOnly("Yaw", std::to_string(cameraYaw));
	gui::TextBoxReadOnly("Pitch", std::to_string(cameraPitch));
	gui::VarDrag("Sensitivity", &cameraSensitivity, 0.05f, 0.05f, 1.0f);
}

GameCameraControllerSystem::GameCameraControllerSystem()
	: System_Internal{ &GameCameraControllerSystem::UpdateGameCameraController }
{
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	//comp.cameraYaw += GameTime::Dt() *180.0f;
	//float pitch = 45.0f;
	
	// Mouse look
	float yaw = comp.cameraYaw;
	float pitch = comp.cameraPitch;
	Vec2 currPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
	if (prevPos.x > 0)
	{
		Vec2 mouseDelta = currPos - prevPos;

		yaw += mouseDelta.x * comp.cameraSensitivity;
		pitch -= mouseDelta.y * comp.cameraSensitivity;

		// Wrap yaw
		if (yaw < 0.0f)
			yaw += 360.0f;
		if (yaw > 360.0f)
			yaw -= 360.0f;

		// Clamp pitch
		if (pitch < -25.0f)
			pitch = -25.0f;
		if (pitch > 1.0f)
			pitch = 1.0f;

		comp.cameraPitch = pitch;
		comp.cameraYaw = yaw;
	}

	prevPos = currPos;
	// This was the best method I could find to rotate the camera with minimal pitch/yaw weirdness
	// I'm not sure why specifically the cam rotation is misbehaving so much, other objects work just fine
	Vec3 eulerAngles{ Vec3{
		/*-pitch * cos(math::ToRadians(comp.cameraYaw)),
		comp.cameraYaw,
		-pitch * sin(math::ToRadians(comp.cameraYaw))*/
		pitch,
		yaw,
		0.0f
	} };
	ecs::GetEntityTransform(&comp).SetWorldRotation(eulerAngles);

	// If no player, we skip the tracking portion
	if (!comp.playerEntity)
		return;

	// Create a look vector
	// TODO: Create forward vector manually bah
	Mat4 rotateMatrix{ math::EulerAnglesToRotationMatrix(eulerAngles) };
	Vec4 forwardVec4{ comp.currentCameraDistance, 0.0f, 0.0f, 0.0f };
	Vec3 forward = Vec3{ rotateMatrix * forwardVec4 };

	// Calculate camera position
	Vec3 playerPos = comp.playerEntity->GetTransform().GetWorldPosition();
	Vec3 cameraPos = playerPos + forward;
	ecs::GetEntity(&comp)->GetTransform().SetWorldPosition(forward);
}
