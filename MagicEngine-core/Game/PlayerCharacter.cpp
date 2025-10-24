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
#include "Game/PlayerCharacter.h"
#include "Game/Character.h"
#include "Game/GameCameraController.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

PlayerMovementComponent::PlayerMovementComponent()
{
}

void PlayerMovementComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("cameraReference", cameraReference);
}

void PlayerMovementComponent::Deserialize(Deserializer& reader)
{
	reader.Deserialize("cameraReference", &cameraReference);
}

void PlayerMovementComponent::EditorDraw()
{
	cameraReference.EditorDraw("Camera");

}

PlayerMovementComponentSystem::PlayerMovementComponentSystem()
	: System_Internal{ &PlayerMovementComponentSystem::UpdatePlayerMovementComponent }

{
}

void PlayerMovementComponentSystem::UpdatePlayerMovementComponent(PlayerMovementComponent& comp)
{
	auto playerEntity = ecs::GetEntity(&comp);
	ecs::CompHandle<physics::PhysicsComp> physicsComp = playerEntity->GetComp<physics::PhysicsComp>();
	ecs::CompHandle<CharacterMovementComponent> characterComp = playerEntity->GetComp<CharacterMovementComponent>();
	Vec2 movement(0.0f, 0.0f);

	// Get inputs
	auto inputInstance = ST<KeyboardMouseInput>::Get();

	ecs::CompHandle< GameCameraControllerComponent> camComp = comp.cameraReference->GetComp< GameCameraControllerComponent>();

	float yawRad = -math::ToRadians(camComp->cameraYaw-90.0f);
	Vec2 camForward = Vec2{ cos(yawRad),sin(yawRad) };
	Vec2 camRight = Vec2{ sin(yawRad),-cos(yawRad) };

	if (inputInstance->GetIsDown(KEY::W))
		movement = movement + camForward;
	if (inputInstance->GetIsDown(KEY::S))
		movement = movement - camForward;
	if (inputInstance->GetIsDown(KEY::D))
		movement = movement + camRight;
	if (inputInstance->GetIsDown(KEY::A))
		movement = movement - camRight;

	if (movement.LengthSqr() > 0.0f)
		movement = movement.Normalized();

	characterComp->SetMovementVector(movement);
}
