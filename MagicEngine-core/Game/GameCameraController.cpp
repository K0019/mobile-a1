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
{
}

//GameCameraControllerComponent::~GameCameraControllerComponent()
//{
//}

void GameCameraControllerComponent::Serialize(Serializer& writer) const
{
}

void GameCameraControllerComponent::Deserialize(Deserializer& reader)
{
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
glm::mat3x3 GetRotationMatrix(Vec3 rotation)
{
	Vec3 rot = rotation;
	float cx = cos(rot.x); // pitch
	float sx = sin(rot.x);
	float cy = cos(rot.y); // yaw
	float sy = sin(rot.y);
	float cz = cos(rot.z); // roll
	float sz = sin(rot.z);

	// Rotation order: Y * X * Z
	return  glm::mat3x3(
		cy * cz + sy * sx * sz, cz * sy * sx - cy * sz, sy * cx,
		cx * sz, cx * cz, -sx,
		cy * sx * sz - cz * sy, cy * cz * sx + sy * sz, cy * cx
	);
}

void GameCameraControllerSystem::UpdateGameCameraController(GameCameraControllerComponent& comp)
{
	// If no player or camera reference, just return.
	if (!comp.playerEntity || !comp.cameraEntity)
		return;

	// Mouse look
	float yaw = comp.cameraYaw;
	float pitch = comp.cameraPitch;
	Vec2 currPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
	if (prevPos.x > 0)
	{
		Vec2 mouseDelta = currPos - prevPos;

		yaw -= mouseDelta.x * comp.cameraSensitivity;
		pitch+= mouseDelta.y * comp.cameraSensitivity;

		// Wrap yaw
		if (yaw < 0.0f)
			yaw += 360.0f;
		if (yaw > 360.0f)
			yaw -= 360.0f;

		// Clamp pitch
		if (pitch < -85.0f)
			pitch = -85.0f;
		if (pitch > 5.0f)
			pitch = 5.0f;

		comp.cameraPitch = pitch;
		comp.cameraYaw = yaw;
	}
	prevPos = currPos;

	// Get the camera's position
	float verticalFactor = sin(math::ToRadians(pitch));
	float horizontalFactor =cos(math::ToRadians(pitch));
	Vec3 calculatedCameraDirection = Vec3(
		horizontalFactor * cos(math::ToRadians(yaw)),
		verticalFactor,
		horizontalFactor * sin(math::ToRadians(yaw))
	);
	calculatedCameraDirection *= math::ToDegrees(1);

	ecs::EntityHandle compEntity = ecs::GetEntity(&comp);
	compEntity->GetTransform().SetWorldRotation(calculatedCameraDirection);
	//Vec2 cameraMovement = comp.lookAction.ConvertToValueType();

	// Find player position
	//Vec3 playerPosition = comp.playerEntity->GetTransform().GetWorldPosition();

	// Update prevPos
}
