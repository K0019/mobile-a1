/******************************************************************************/
/*!
\file   Character.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	CharacterMovementComponent is an ECS component-system pair which controls character movement.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/Character.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

CharacterMovementComponent::CharacterMovementComponent()
	: movementVector{ 0.0f,0.0f }
	, hitDebugObject{ nullptr }
	, moveSpeed{ 0.0f }
	, rotateSpeed{ 0.0f }
	, stunTimePerHit{ 0.0f }
	, heldItem{ nullptr }
	, currentStunTime{ 0.0f }
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

void CharacterMovementComponent::RotateTowards(Vec2 vector)
{
	auto characterEntity = ecs::GetEntity(this);
	Transform& characterTransform = characterEntity->GetTransform();

	Vec3 currentRotation = characterTransform.GetWorldRotation();

	float targetAngle = math::ToDegrees(atan2(vector.y, vector.x));
	float newAngle = math::MoveTowardsAngle(currentRotation.y, targetAngle, rotateSpeed * GameTime::Dt());
	currentRotation.y = newAngle;

	characterTransform.SetWorldRotation(currentRotation);
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

void CharacterMovementComponent::Throw(Vec3 direction)
{
	if (heldItem == nullptr)
		return;

	auto tmpItem = heldItem;
	DropItem();

	tmpItem->GetComp<physics::PhysicsComp>()->SetLinearVelocity(direction*10.0f);
}

void CharacterMovementComponent::GrabItem(ecs::CompHandle<GrabbableItemComponent> item)
{
	// Sanity check!
	if (item == nullptr)
		return;

	item->isHeld = true;
	item->owner = ecs::GetEntity(this);
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
	writer.Serialize("stunTimePerHit", stunTimePerHit);
	writer.Serialize("groundFriction", groundFriction);

	writer.Serialize("hitDebugObject",hitDebugObject);
	writer.Serialize("heldItem",heldItem);
}

void CharacterMovementComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("moveSpeed", &moveSpeed);
	reader.DeserializeVar("rotateSpeed", &rotateSpeed);
	reader.DeserializeVar("stunTimePerHit", &stunTimePerHit);
	reader.DeserializeVar("groundFriction", &groundFriction);

	reader.DeserializeVar("hitDebugObject", &hitDebugObject);
	reader.DeserializeVar("heldItem", &heldItem);
}

void CharacterMovementComponent::EditorDraw()
{
	gui::VarInput("Move Speed", &moveSpeed);
	gui::VarInput("Rotation Speed", &rotateSpeed);
	gui::VarInput("Stun Time Per Hit", &stunTimePerHit);
	gui::VarInput("Ground Friction", &groundFriction);

	hitDebugObject.EditorDraw("Hit Debug Object");
	heldItem.EditorDraw("Held Item");
}

CharacterMovementComponentSystem::CharacterMovementComponentSystem()
	: System_Internal{ &CharacterMovementComponentSystem::UpdateCharacterMovementComponent }

{
}



void CharacterMovementComponentSystem::UpdateCharacterMovementComponent(CharacterMovementComponent& comp)
{
	auto characterEntity = ecs::GetEntity(&comp);
	Transform& characterTransform = characterEntity->GetTransform();

	// Update held item
	if (auto heldItem{ comp.heldItem })
	{
		// Transform related
		heldItem->GetTransform().SetParent(characterTransform);
		heldItem->GetTransform().SetLocalPosition(Vec3{ 0,0,1 });
		heldItem->GetTransform().SetLocalRotation(Vec3{ 0,5,10 });
		heldItem->GetComp<GrabbableItemComponent>()->owner = characterEntity;
		heldItem->GetComp<GrabbableItemComponent>()->isHeld = true;
	}

	// Perform stun check
	ecs::CompHandle<physics::PhysicsComp> physicsComp = characterEntity->GetComp<physics::PhysicsComp>();
	Vec3 currVel = physicsComp->GetLinearVelocity();
	if (comp.currentStunTime > 0.0f)
	{
		comp.currentStunTime -= GameTime::Dt();

		// Can only come out of stun when on the ground
		if (math::Abs(currVel.y) > 0.01f && comp.currentStunTime < 0.0f)
			comp.currentStunTime = GameTime::Dt();

		return;
	}


	// Get inputs
	Vec2 movement = comp.GetMovementVector();

	// Normalize the move vector if it's over 1.0f in length
	if (movement.LengthSqr() > 1.0f)
		movement = movement.Normalized();

	// Apply friction
	Vec3 drag{ -currVel.x,0.0f,-currVel.z };
	float groundSpeed = drag.Length();
	if (groundSpeed <= comp.groundFriction)
	{
		physicsComp->AddLinearVelocity(drag);
	}
	else
	{
		physicsComp->AddLinearVelocity(drag.Normalized() * comp.groundFriction * groundSpeed);
	}

	// Apply input movement
	Vec3 moveDir = Vec3{ movement.x * comp.moveSpeed,0.0f,-movement.y * comp.moveSpeed };
	physicsComp->AddLinearVelocity(moveDir);

	if (movement.LengthSqr() > 0.0f)
		comp.RotateTowards(movement);

}
