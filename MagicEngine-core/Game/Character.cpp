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
\par    DigiPen login: m.chsn

\brief
	GameCameraController is an ECS component-system pair which takes control of
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the character, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/Character.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

CharacterMovementComponent::CharacterMovementComponent()
{
}

const Vec2 CharacterMovementComponent::GetMovementVector()
{
	return movementVector;
}

void CharacterMovementComponent::SetMovementVector(Vec2 vector)
{
	movementVector = vector;
}

void CharacterMovementComponent::DropItem()
{
	// Sanity check!
	if (heldItem == nullptr)
		return;

	// Physics Comp related
	heldItem->GetComp<physics::PhysicsComp>()->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, true);
	heldItem->GetComp<GrabbableItemComponent>()->isHeld = false;

	// Transform related
	heldItem->GetTransform().SetParent(nullptr);

	heldItem = nullptr;
}

void CharacterMovementComponent::GrabItem(ecs::CompHandle<GrabbableItemComponent> item)
{
	// Sanity check!
	if (item == nullptr)
		return;

	item->isHeld = true;
	heldItem = ecs::GetEntity(item);

	// Physics Comp related
	heldItem->GetComp<physics::PhysicsComp>()->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, false);

}

void CharacterMovementComponent::Attack()
{
	// No item, no attack (again different from the proto but we can change it later
	if (heldItem == nullptr)
		return;

	// Hard-code a simple start point etc for now
	Vec3 rotation = ecs::GetEntity(this)->GetTransform().GetWorldRotation();
	Vec3 direction(sin(math::ToRadians(rotation.y+90)), 0, cos(math::ToRadians(rotation.y+90)));
	Vec3 startPoint = ecs::GetEntity(this)->GetTransform().GetWorldPosition() + direction*2.5f;
	
	if(hitDebugObject!=nullptr)
	{
		hitDebugObject->GetTransform().SetWorldPosition(startPoint);
		hitDebugObject->GetTransform().SetWorldRotation(Vec3(0.0f, math::ToDegrees(atan2(direction.x,direction.z)), 0.0f));
	}
	// Call Attack from the GrabbableItem component
	heldItem->GetComp<GrabbableItemComponent>()->Attack(startPoint, direction);
}

void CharacterMovementComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("moveSpeed", moveSpeed);
	writer.Serialize("rotateSpeed", rotateSpeed);
	writer.Serialize(hitDebugObject);
}

void CharacterMovementComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("moveSpeed", &moveSpeed);
	reader.DeserializeVar("rotateSpeed", &rotateSpeed);
	reader.Deserialize(&hitDebugObject);
}

void CharacterMovementComponent::EditorDraw()
{
	gui::VarInput("Move Speed", &moveSpeed);
	gui::VarInput("Rotation Speed", &rotateSpeed);
	hitDebugObject.EditorDraw("Hit Debug Object");
}

CharacterMovementComponentSystem::CharacterMovementComponentSystem()
	: System_Internal{ &CharacterMovementComponentSystem::UpdateCharacterMovementComponent }

{
}



void CharacterMovementComponentSystem::UpdateCharacterMovementComponent(CharacterMovementComponent& comp)
{
	auto characterEntity = ecs::GetEntity(&comp);
	ecs::CompHandle<physics::PhysicsComp> physicsComp = characterEntity->GetComp<physics::PhysicsComp>();

	// Get inputs
	Vec2 movement = comp.GetMovementVector();

	// Normalize the move vector if it's overr 1.0f in length
	if (movement.LengthSqr()>1.0f)
		movement = movement.Normalized();

	// Apply input movement
	Vec3 currVel = physicsComp->GetLinearVelocity();
	Vec3 moveDir = Vec3{ movement.x * comp.moveSpeed,currVel.y,-movement.y * comp.moveSpeed};
	physicsComp->SetLinearVelocity(moveDir);
	//physicsComp->SetAngularVelocity(Vec3{ 0.0f });

	Transform& characterTransform = characterEntity->GetTransform();
	Vec3 currentRotation = characterTransform.GetWorldRotation();

	// Debug line here, not sure why the angle is having issues when being rotated past 90 degrees
	//characterTransform.SetWorldRotation(Vec3{ 0.0f,90.0f,0.0f });

	// Commented out for testing, these *should* work.
	float newAngle = currentRotation.y;
	Vec3 rotation{ 0.0f,newAngle ,0.0f };
	// Handle rotation
	if (movement.LengthSqr() > 0.0f)
	{
		float targetAngle = math::ToDegrees(atan2(movement.y, movement.x));
		newAngle = math::MoveTowardsAngle(currentRotation.y, targetAngle, comp.rotateSpeed * GameTime::Dt());
		rotation.y = newAngle;
	}
	CONSOLE_LOG(LogLevel::LEVEL_DEBUG) << rotation.y;
	characterTransform.SetWorldRotation(rotation);

	// Update held item
	if (auto heldItem{ comp.heldItem })
	{
		// Transform related
		heldItem->GetTransform().SetParent(characterTransform);
		heldItem->GetTransform().SetLocalPosition(Vec3{ 0,0,1 });
		heldItem->GetTransform().SetLocalRotation(Vec3{ 0,5,10 });
	}
}
