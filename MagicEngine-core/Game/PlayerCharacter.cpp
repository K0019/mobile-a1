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
#include "Game/PlayerCharacter.h"
#include "Game/Character.h"
#include "Game/GameCameraController.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

PlayerMovementComponent::PlayerMovementComponent()
	: grabDistance{ 0.0f }
	, cameraReference{nullptr}
{
}

void PlayerMovementComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("cameraReference", cameraReference);
	writer.Serialize("grabDistance", grabDistance);
}

void PlayerMovementComponent::Deserialize(Deserializer& reader)
{
	reader.Deserialize("cameraReference", &cameraReference);
	reader.DeserializeVar("grabDistance", &grabDistance);
}

void PlayerMovementComponent::EditorDraw()
{
	cameraReference.EditorDraw("Camera");
	gui::VarInput("Grab Distance", &grabDistance);
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

	float yawRad = math::ToRadians(-camComp->cameraYaw+90.0f);
	Vec2 camForward = Vec2{ cos(yawRad),sin(yawRad) };
	Vec2 camRight = Vec2{ -sin(yawRad),cos(yawRad) };

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

	// Grabbing items
	if (inputInstance->GetValue(INPUT_READ_TYPE::CURRENT, KEY::F))
	{
		if (characterComp->heldItem == nullptr)
		{

			float closestDistance = comp.grabDistance * comp.grabDistance;
			ecs::CompHandle< GrabbableItemComponent> closestItem = nullptr;
			for (auto itemComp = ecs::GetCompsBegin<GrabbableItemComponent>(); itemComp != ecs::GetCompsEnd<GrabbableItemComponent>(); ++itemComp)
			{
				// Just in case, this shouldn't happen
				if (itemComp.GetEntity() == nullptr)
					continue;

				// Can't pick up other held items
				if (itemComp->isHeld == true)
					continue;

				// Distance check
				Vec3 direction = itemComp.GetEntity()->GetTransform().GetWorldPosition() - playerEntity->GetTransform().GetWorldPosition();
				if (direction.LengthSqr() < closestDistance)
				{
					closestItem = itemComp.GetCompHandle();
					closestDistance = direction.LengthSqr();
				}
			}

			if (closestItem != nullptr)
			{
				characterComp->DropItem();
				characterComp->GrabItem(closestItem);
			}
		}
	}

	// Throw item
	if (inputInstance->GetValue(INPUT_READ_TYPE::CURRENT, KEY::B))
	{
		// Look for the nearest enemy
		Vec3 throwDirection{ camForward.x,1.0f,-(camForward.y ) };

		characterComp->Throw(throwDirection);
	}

	// Doesn't seem to be working again
	if (inputInstance->GetIsPressed(KEY::M1))
		characterComp->Attack();

	characterComp->SetMovementVector(movement);

	if (inputInstance->GetIsDown(KEY::LSHIFT))
		characterComp->Dodge(movement);
}
