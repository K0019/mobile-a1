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
#include "Scripting//ScriptComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Game/Delusion.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

PlayerMovementComponent::PlayerMovementComponent()
	: grabDistance{ 0.0f }
	, cameraReference{nullptr}
	, ultimateAttackDamage{}
	, isUltimateAttack{false}
{
}

void PlayerMovementComponent::Serialize(Serializer& writer) const
{
	ISerializeable::Serialize(writer);
	writer.Serialize("cameraReference", cameraReference);
	writer.Serialize("testReference", testReference);
	writer.Serialize("grabDistance", grabDistance);
}

void PlayerMovementComponent::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);
	reader.Deserialize("cameraReference", &cameraReference);
	reader.Deserialize("testReference", &testReference);
	reader.DeserializeVar("grabDistance", &grabDistance);
}

void PlayerMovementComponent::EditorDraw()
{
	cameraReference.EditorDraw("Camera");
	testReference.EditorDraw("Test");
	gui::VarInput("Grab Distance", &grabDistance);
	gui::VarInput("Ultimate Attack Damage", &ultimateAttackDamage);
}

void PlayerMovementComponent::Parry()
{
	auto characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() };
	characterComp->Parry();
}


void PlayerMovementComponent::UltimateAttack()
{
	auto delutionComp{ ecs::GetEntity(this)->GetComp<DelusionComponent>() };
	auto characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() };
	if (!delutionComp || !characterComp ||delutionComp->GetCurrDelusionTier() != DELUSION_TIER::APLUS)
		return;

	isUltimateAttack = true;
	characterComp->Attack();
	delutionComp->SetDelusion(0.f);
	CONSOLE_LOG(LEVEL_INFO) << "Ultimate Attack.";
}

PlayerMovementComponentSystem::PlayerMovementComponentSystem()
	: System_Internal{ &PlayerMovementComponentSystem::UpdatePlayerMovementComponent }

{
}

void PlayerMovementComponentSystem::UpdatePlayerMovementComponent(PlayerMovementComponent& comp)
{
	auto playerEntity = ecs::GetEntity(&comp);
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

#ifdef __ANDROID__

	auto* input = ST<MagicInput>::Get();
	if (input) {
		// Action name must match what is created in the Input Binding (“AndroidJoystick”)
		if (auto a = input->GetAction<Vec2>("AndroidJoystick")) {
			Vec2 joy = a->GetValue();        // expected in [-1,1], x=right, y=up
			Vec2 tempMove = camForward * joy.y + camRight * joy.x;  // note += so it blends with WASD
			movement = movement + tempMove;
		}
	}
#endif


	if (movement.LengthSqr() > 0.0f)
		movement = movement.Normalized();

	float speedMult = 1.0f;
	// If attacking, we apply the speed multiplier
	if (characterComp->isAttacking)
	{
		speedMult = characterComp->attackingMoveSpeedMultiplier;
	}
	movement = movement * speedMult;

	// Grabbing items
	if (inputInstance->GetIsPressed(KEY::E) || EventsReader<Events::GameActionGrabItem>{}.ExtractEvent())
	{
		float closestDistance = comp.grabDistance * comp.grabDistance;
		ecs::CompHandle< GrabbableItemComponent> closestItem = nullptr;
		for (auto itemComp = ecs::GetCompsBegin<GrabbableItemComponent>(); itemComp != ecs::GetCompsEnd<GrabbableItemComponent>(); ++itemComp)
		{
			// Just in case, don't grab nothing
			if (itemComp.GetEntity() == nullptr)
				continue;

			// Don't grab self
			if (itemComp.GetEntity() == playerEntity)
				continue;

			// Don't grab people
			if (itemComp.GetEntity()->GetComp<CharacterMovementComponent>())
				continue;

			assert(ecs::IsEntityHandleValid(itemComp.GetEntity()));

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

	// Throw item
	if (inputInstance->GetIsPressed(KEY::Q) || EventsReader<Events::GameActionThrowItem>{}.ExtractEvent())
	{
		// Look for the nearest enemy
		Vec3 throwDirection{ camForward.x,0.f,camForward.y  };

		characterComp->Throw(throwDirection);
	}

	if (inputInstance->GetIsPressed(KEY::M1) || EventsReader<Events::GameActionAttack>{}.ExtractEvent())
		characterComp->Attack();

	if (inputInstance->GetIsPressed(KEY::R))
		comp.UltimateAttack();

	characterComp->SetMovementVector(movement);

	if (inputInstance->GetIsDown(KEY::LSHIFT) || EventsReader<Events::GameActionDodge>{}.ExtractEvent())
		characterComp->Dodge(movement);


	if (inputInstance->GetIsPressed(KEY::LCTRL))
		comp.Parry();
}
