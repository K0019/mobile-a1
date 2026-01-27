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
#include "Physics/JoltPhysics.h"
#include "ECS/EntityUID.h"
#include "ECS/IEditorComponent.h"
#include "Game/GrabbableItem.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"
#include "Engine/AnimatorStateMachine.h"



using CharacterRef = JPH::Ref<JPH::CharacterVirtual>;

/*****************************************************************//*!
\class CharacterMovementComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class CharacterMovementComponent
	: public IRegisteredComponent<CharacterMovementComponent>
	, public IEditorComponent<CharacterMovementComponent>
	, public ecs::IComponentCallbacks
{
private:
	Vec2 movementVector;
public:
	UserResourceHandle<ResourceAnimation> animations[ANIMATION_TYPES::ANIM_TOTAL];

	CharacterRef joltCharRef;
	EntityReference hitDebugObject;
	EntityReference heldItem;
	Vec3 center;
	float radius;
	float height;
	float moveSpeed;
	float rotateSpeed;
	float stunTimePerHit;
	float groundFriction;
	float dodgeCooldown;
	float dodgeDuration;
	float dodgeSpeed;
	float throwPower;
	float parryTime;
	float parryCoolDownTime;
	float parryDelusion;
	float currParryCoolDown;
	float currParryTime;

	std::string dodgeSound;
	std::string attackSound;
	std::string hurtSound;

	// Not serialized
	float currentStunTime;
	float currentDodgeTime;
	float currentDodgeCooldown;
	float speedMultiplier;

	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	CharacterMovementComponent();

	void OnCreation() override;
	void OnAttached() override;
	void OnDetached() override;

	const Vec2 GetMovementVector();
	bool Dodge(Vec2 vector);
	void SetMovementVector(Vec2 vector);
	void RotateTowards(Vec2 vector);

	void DropItem();
	void Throw(Vec3 direction);
	void GrabItem(ecs::CompHandle<GrabbableItemComponent> item);
	void Attack();
	bool IsAttacking() const;
	bool IsDodging();
	bool IsParrying();
	void OnParrySuccess();
	void Parry();

	// Gets the GrabbableItem that this character is holding. If not holding anything, returns the GrabbableItem on the entity itself.
	ecs::CompHandle<GrabbableItemComponent> GetHeldItem();

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	void SetSpeedMultiplier(float mult);
	void ResetSpeedMultiplier();

	void SetCenter(const Vec3& vec);

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
	property_var(center),
	property_var(radius),
	property_var(height),
	property_var(moveSpeed),
	property_var(rotateSpeed),
	property_var(throwPower),
	property_var(parryCoolDownTime),
	property_var(parryTime),
	property_var(parryDelusion),
	property_var(dodgeSound),
	property_var(attackSound),
	property_var(hurtSound),
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

	void ApplyAttack(size_t moveIndex, const Transform& transform, CharacterMovementComponent& charComp);

public:
	bool PreRun() override;
	void PostRun() override;
};