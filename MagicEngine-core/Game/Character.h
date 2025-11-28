/******************************************************************************/
/*!
\file   Character.h
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
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"
#include "Game/GrabbableItem.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

#define ANIMATIONS \
X(IDLE,     "Idle")\
X(WALK,     "Walk")\
X(ATTACK,   "Attack")\
X(HURT,   "Hurt")\
X(DODGE,   "Dodge")\
X(THROW,   "Throw")\

#define X(type, name) type,
enum ANIMATION_TYPES:size_t
{
	ANIMATIONS
	ANIM_TOTAL
};
#undef X


/*****************************************************************//*!
\class CharacterMovementComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class CharacterMovementComponent
	: public IRegisteredComponent<CharacterMovementComponent>
	, public IEditorComponent<CharacterMovementComponent>
{
private:
	Vec2 movementVector;
public:
	UserResourceHandle<ResourceAnimation> animations[ANIMATION_TYPES::ANIM_TOTAL];

	EntityReference hitDebugObject;
	EntityReference heldItem;
	float moveSpeed;
	float rotateSpeed;
	float stunTimePerHit;
	float groundFriction;
	float dodgeCooldown;
	float dodgeDuration;
	float dodgeSpeed;

	// Not serialized
	float currentStunTime;
	float currentDodgeTime;
	float currentDodgeCooldown;
	bool isAttacking;
	float speedMultiplier;

	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	CharacterMovementComponent();

	const Vec2 GetMovementVector();
	bool Dodge(Vec2 vector);
	void SetMovementVector(Vec2 vector);
	void RotateTowards(Vec2 vector);

	void DropItem();
	void Throw(Vec3 direction);
	void GrabItem(ecs::CompHandle<GrabbableItemComponent> item);
	bool Attack();
	bool IsDodging();

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	void SetSpeedMultiplier(float mult);
	void ResetSpeedMultiplier();

	property_vtable()

	// ===== Lua helpers =====
public:
	float GetMoveSpeed() const { return moveSpeed; }
	void  SetMoveSpeed(float v) { moveSpeed = v; }

	float GetRotateSpeed() const { return rotateSpeed; }
	void  SetRotateSpeed(float v) { rotateSpeed = v; }

	float GetStunTimePerHit() const { return stunTimePerHit; }
	void  SetStunTimePerHit(float v) { stunTimePerHit = v; }

	float GetGroundFriction() const { return groundFriction; }
	void  SetGroundFriction(float v) { groundFriction = v; }

	float GetDodgeCooldown() const { return dodgeCooldown; }
	void  SetDodgeCooldown(float v) { dodgeCooldown = v; }

	float GetDodgeDuration() const { return dodgeDuration; }
	void  SetDodgeDuration(float v) { dodgeDuration = v; }

	float GetDodgeSpeed() const { return dodgeSpeed; }
	void  SetDodgeSpeed(float v) { dodgeSpeed = v; }

	float GetCurrentStunTime() const { return currentStunTime; }
	void  SetCurrentStunTime(float v) { currentStunTime = v; }

	float GetCurrentDodgeTime() const { return currentDodgeTime; }
	void  SetCurrentDodgeTime(float v) { currentDodgeTime = v; }

	float GetCurrentDodgeCooldown() const { return currentDodgeCooldown; }
	void  SetCurrentDodgeCooldown(float v) { currentDodgeCooldown = v; }

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

};

property_begin(CharacterMovementComponent)
{
	property_var(moveSpeed),
	property_var(rotateSpeed)
}
property_vend_h(CharacterMovementComponent)

/*****************************************************************//*!
\class CharacterMovementComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class CharacterMovementComponentSystem : public ecs::System<CharacterMovementComponentSystem, CharacterMovementComponent>
{
public:
	CharacterMovementComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates CharacterMovementComponent.
	\param comp
		The CharacterMovementComponent to update.
	*//******************************************************************/
	void UpdateCharacterMovementComponent(CharacterMovementComponent& comp);
};