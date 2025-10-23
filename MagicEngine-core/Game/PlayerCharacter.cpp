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
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

PlayerMovementComponent::PlayerMovementComponent()
{
}


void PlayerMovementComponent::EditorDraw()
{
	gui::VarInput("Move Speed", &moveSpeed);
	gui::VarInput("Rotation Speed", &rotateSpeed);
}

PlayerMovementComponentSystem::PlayerMovementComponentSystem()
	: System_Internal{ &PlayerMovementComponentSystem::UpdatePlayerMovementComponent }

{
}

void PlayerMovementComponentSystem::UpdatePlayerMovementComponent(PlayerMovementComponent& comp)
{
	auto playerEntity = ecs::GetEntity(&comp);
	ecs::CompHandle<physics::PhysicsComp> physicsComp = playerEntity->GetComp<physics::PhysicsComp>();
	Vec2 movement(0.0f, 0.0f);

	// Get inputs
	auto inputInstance = ST<KeyboardMouseInput>::Get();
	if (inputInstance->GetIsDown(KEY::W))
		movement.y += 1.0f;
	if (inputInstance->GetIsDown(KEY::S))
		movement.y -= 1.0f;
	if (inputInstance->GetIsDown(KEY::D))
		movement.x += 1.0f;
	if (inputInstance->GetIsDown(KEY::A))
		movement.x -= 1.0f;

	if (movement.LengthSqr() > 0.0f)
		movement = movement.Normalized();

	// Apply input movement
	Vec3 currVel = physicsComp->GetLinearVelocity();
	Vec3 moveDir = Vec3{ movement.x * comp.moveSpeed,currVel.y,-movement.y * comp.moveSpeed };
	physicsComp->SetLinearVelocity(moveDir);
	//physicsComp->SetAngularVelocity(Vec3{ 0.0f });

	Transform& playerTransform = playerEntity->GetTransform();
	Vec3 currentRotation = playerTransform.GetWorldRotation();

	// Debug line here, not sure why the angle is having issues when being rotated past 90 degrees
	playerTransform.SetWorldRotation(currentRotation);

	// Commented out for testing, these *should* work.
	//float newAngle = currentRotation.y;
	//Vec3 rotation{ 0.0f,newAngle ,0.0f };
	//// Handle rotation
	//if (movement.LengthSqr() > 0.0f)
	//{
	//	float targetAngle = math::ToDegrees(atan2(movement.y, movement.x));
	//	newAngle = math::MoveTowardsAngle(currentRotation.y, targetAngle, comp.rotateSpeed * GameTime::Dt());
	//	rotation.y = newAngle;
	//}
	//	CONSOLE_LOG(LogLevel::LEVEL_DEBUG) << rotation.y;
	//playerTransform.SetWorldRotation(rotation);
}
