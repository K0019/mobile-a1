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

PlayerMovementComponent::PlayerMovementComponent()
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

PlayerMovementComponent::~PlayerMovementComponent()
{
}

void PlayerMovementComponent::SetOffsetCurrent(float offset)
{
	offsetAmountCurrent = offset;
}

void PlayerMovementComponent::EditorDraw()
{
}

void PlayerMovementComponentSystem::UpdatePlayerMovementComponent(PlayerMovementComponent& comp)
{
	if (comp.playerEntity == nullptr)
		comp.playerEntity = ecs::GetEntity(&comp);

	ecs::CompHandle<physics::PhysicsComp> physicsComp = comp.playerEntity->GetComp<physics::PhysicsComp>();
	Vec2 movement( 0.0f,0.0f );
	auto inputInstance = ST<KeyboardMouseInput>::Get();
	if (inputInstance->GetIsDown(KEY::W))
		movement.y += 1.0f;
	if (inputInstance->GetIsDown(KEY::S))
		movement.y -= 1.0f;
	if (inputInstance->GetIsDown(KEY::D))
		movement.x += 1.0f;
	if (inputInstance->GetIsDown(KEY::A))
		movement.x -= 1.0f;

	physicsComp->SetLinearVelocity(Vec3{ movement.x,0.0f,movement.y });
	//physicsComp.

	
}
