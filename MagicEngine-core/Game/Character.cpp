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
#include "Components/NameComponent.h"
#include "Game/Character.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Graphics/AnimationComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Audio.h"
#include "Managers\AudioManager.h"

#define X(type, name) name,
char const* animNames[] =
{
	ANIMATIONS
};
#undef X

CharacterMovementComponent::CharacterMovementComponent()
	: movementVector{ 0.0f,0.0f }
	, hitDebugObject{ nullptr }
	, moveSpeed{ 0.0f }
	, rotateSpeed{ 0.0f }
	, stunTimePerHit{ 0.0f }
	, groundFriction{ 0.0f }
	, dodgeCooldown{ 0.0f }
	, dodgeDuration{ 0.0f }
	, dodgeSpeed{ 0.0f }
	, heldItem{ nullptr }
	, currentStunTime{ 0.0f }
	, currentDodgeTime{ 0.0f }
	, isAttacking{ false }
{
}

const Vec2 CharacterMovementComponent::GetMovementVector()
{
	return movementVector;
}

void CharacterMovementComponent::Dodge(Vec2 vector)
{
	// Don't allow dodging if still on cooldown
	if (currentDodgeCooldown > 0.0f)
		return;

	// Don't dodge nowhere
	if (vector.LengthSqr() == 0.0f)
		return;	

	SetMovementVector(vector.Normalized());
	currentDodgeTime = dodgeDuration;
	currentDodgeCooldown = dodgeCooldown;
}

void CharacterMovementComponent::SetMovementVector(Vec2 vector)
{
	// Can't change movement direction if dodging
	if (currentDodgeTime > 0.0f)
		return;

	movementVector = vector;
}

void CharacterMovementComponent::RotateTowards(Vec2 vector)
{
	auto characterEntity = ecs::GetEntity(this);
	Transform& characterTransform = characterEntity->GetTransform();

	Vec3 currentRotation = characterTransform.GetWorldRotation();

	float targetAngle = math::ToDegrees(atan2(-vector.y, vector.x));
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

	tmpItem->GetComp<physics::PhysicsComp>()->SetLinearVelocity(direction);
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

	// Play Audio
	ST<AudioManager>::Get()->PlaySound3D("weapon pickup "+std::to_string(randomRange<int>(1,4)), false, ecs::GetEntity(this)->GetTransform().GetWorldPosition());
}

bool CharacterMovementComponent::Attack()
{
	ecs::EntityHandle attackItem{ heldItem };

	// If already in attack animation, skip
	if (isAttacking)
		return false;

	// If not holding an item, we fallback to the character's entity itself
	if (attackItem == nullptr && ecs::GetEntity(this)->GetComp<GrabbableItemComponent>())
	{
		attackItem = ecs::GetEntity(this);
	}

	// If the entity doesn't have a GI comp, then it has no unarmed attack.
	else if (attackItem == nullptr)
		return false;

	// Audio plays here
	//if (auto audioSourceComp{ ecs::GetEntity(this)->GetComp<AudioSourceComponent>() })
	//{
	//	//audioSourceComp->Set
	//}

	// I shall perform a hackery
	ST<AudioManager>::Get()->PlaySound3D("Attack", false, ecs::GetEntity(this)->GetTransform().GetWorldPosition());

	ecs::EntityHandle thisEntity = ecs::GetEntity(this);

	ST<Scheduler>::Get()->Add(attackItem->GetComp<GrabbableItemComponent>()->attackDelay, [attackItem, thisEntity, thisComp = this]() {

		// If the attack animation was cancelled, we cancel this task as well
		if (!thisComp->isAttacking)
			return;

	// Hard-code a simple start point etc for now
	Vec3 rotation = thisEntity->GetTransform().GetWorldRotation();
	Vec3 direction(sin(math::ToRadians(rotation.y + 90)), 0, cos(math::ToRadians(rotation.y + 90)));
	Vec3 startPoint = thisEntity->GetTransform().GetWorldPosition() + direction;

	auto hitDebugObject = thisComp->hitDebugObject;
	if (hitDebugObject != nullptr)
	{
		hitDebugObject->GetTransform().SetWorldPosition(startPoint);
		hitDebugObject->GetTransform().SetWorldRotation(Vec3(0.0f, math::ToDegrees(atan2(direction.x, direction.z)), 0.0f));
		hitDebugObject->GetTransform().SetWorldScale(attackItem->GetComp<GrabbableItemComponent>()->attackBox);
	}


	// Call Attack from the GrabbableItem component
	attackItem->GetComp<GrabbableItemComponent>()->Attack(startPoint, direction); });
	
	// Get the animation component
	ecs::CompHandle<AnimationComponent> animComp = thisEntity->GetComp<AnimationComponent>();
	animComp->animHandleA = animations[ATTACK];
	isAttacking = true;

	return true;
}

void CharacterMovementComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("moveSpeed", moveSpeed);
	writer.Serialize("rotateSpeed", rotateSpeed);
	writer.Serialize("stunTimePerHit", stunTimePerHit);
	writer.Serialize("groundFriction", groundFriction);

	writer.Serialize("dodgeCooldown", dodgeCooldown);
	writer.Serialize("dodgeDuration", dodgeDuration);
	writer.Serialize("dodgeSpeed", dodgeSpeed);


	writer.Serialize("hitDebugObject", hitDebugObject);
	writer.Serialize("heldItem", heldItem);

	size_t maxArraySize = ANIM_TOTAL;

	// Load all animations
	writer.StartArray("animations");
	for (size_t i = 0; i < maxArraySize; ++i)
	{
		writer.StartObject();
		writer.Serialize(animations[i]);
		writer.EndObject();
	}
	writer.EndArray();
}

void CharacterMovementComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("moveSpeed", &moveSpeed);
	reader.DeserializeVar("rotateSpeed", &rotateSpeed);
	reader.DeserializeVar("stunTimePerHit", &stunTimePerHit);
	reader.DeserializeVar("groundFriction", &groundFriction);

	reader.DeserializeVar("dodgeCooldown", &dodgeCooldown);
	reader.DeserializeVar("dodgeDuration", &dodgeDuration);
	reader.DeserializeVar("dodgeSpeed", &dodgeSpeed);


	reader.DeserializeVar("hitDebugObject", &hitDebugObject);
	reader.DeserializeVar("heldItem", &heldItem);

	// Load animations

	if (reader.PushAccess("animations"))
	{
		for (size_t i = 0; i < ANIM_TOTAL; ++i)
		{
			if (reader.PushArrayElementAccess(i))
			{
				reader.Deserialize(&animations[i]);
				reader.PopAccess();
			}
		}
		reader.PopAccess();
	}
}

void CharacterMovementComponent::EditorDraw()
{
	gui::VarInput("Move Speed", &moveSpeed);
	gui::VarInput("Rotation Speed", &rotateSpeed);
	gui::VarInput("Stun Time Per Hit", &stunTimePerHit);
	gui::VarInput("Ground Friction", &groundFriction);

	gui::VarInput("Dodge Cooldown", &dodgeCooldown);
	gui::VarInput("Dodge Duration", &dodgeDuration);
	gui::VarInput("Dodge Speed", &dodgeSpeed);

	hitDebugObject.EditorDraw("Hit Debug Object");
	heldItem.EditorDraw("Held Item");

	// Animation input
	for(uint32_t animIndex = 0;animIndex< ANIM_TOTAL;++animIndex)
	{
		const std::string* clip1Name{ ST<MagicResourceManager>::Get()->Editor_GetName(animations[animIndex].GetHash()) };
		gui::TextUnformatted(std::string(animNames[animIndex]));
		gui::SameLine();
		gui::TextBoxReadOnly(std::string("##AnimClip"+std::to_string(animIndex)).c_str(), clip1Name ? clip1Name->c_str() : "");
		gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
			animations[animIndex] = hash;
			});
	}
}

CharacterMovementComponentSystem::CharacterMovementComponentSystem()
	: System_Internal{ &CharacterMovementComponentSystem::UpdateCharacterMovementComponent }
{
}



void CharacterMovementComponentSystem::UpdateCharacterMovementComponent(CharacterMovementComponent& comp)
{
	auto characterEntity = ecs::GetEntity(&comp);
	Transform& characterTransform = characterEntity->GetTransform();

	// Get the animation component
	ecs::CompHandle<AnimationComponent> animComp = characterEntity->GetComp<AnimationComponent>();

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

		if (animComp->animHandleA.GetHash() != comp.animations[HURT].GetHash())
		{
			animComp->animHandleB = animComp->animHandleA;
			animComp->animHandleA = comp.animations[HURT];
		}
		animComp->timeA = comp.currentStunTime / comp.stunTimePerHit;
		comp.currentDodgeTime = 0.0f;
		return;
	}


	// Get inputs
	Vec2 movement = comp.GetMovementVector();

	// Normalize the move vector if it's over 1.0f in length
	if (movement.LengthSqr() > 0.0f)
	{
		if(comp.isAttacking)

		animComp->animHandleB = comp.animations[WALK];
		else
		animComp->animHandleA = comp.animations[WALK];
	}
	else
	{
		if (!comp.isAttacking)
		animComp->animHandleA = comp.animations[IDLE];
	}

	if(comp.isAttacking)
	{
		animComp->loop = false;
		if (animComp->timeA >= animComp->GetClipDuration(animComp->GetAnimationClipA()))
		{
			comp.isAttacking = false;
			animComp->loop = true;
		}
	}
	

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
	Vec3 moveDir = Vec3{ movement.x ,0.0f,movement.y };

	// If dodging, move faster
	if (comp.currentDodgeTime > 0.0f)
	{
		comp.currentDodgeTime -= GameTime::Dt();
		moveDir *= comp.dodgeSpeed;
		animComp->animHandleA = comp.animations[DODGE];
	}
	else
	{
		moveDir *= comp.moveSpeed;
	}

	physicsComp->AddLinearVelocity(moveDir);

	comp.currentDodgeCooldown -= GameTime::Dt();

	if (movement.LengthSqr() > 0.0f)
		comp.RotateTowards(movement);

	// Handle dodge "animation"
	// Lord forgive me for I have committed jank
	//auto children = characterTransform.GetChildren();
	//for (auto child : children)
	//{
	//	Vec3 localRot = child->GetLocalRotation();
	//	if (comp.currentDodgeTime > 0.0f)
	//	{
	//		localRot.x = 360.0f * (1.0f - (comp.currentDodgeTime / comp.dodgeDuration));
	//	}
	//	else
	//	{
	//		localRot.x = 0.0f;
	//	}
	//	child->SetLocalRotation(localRot);
	//}
}
