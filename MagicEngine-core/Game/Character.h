/******************************************************************************/
/*!
\file   CharacterCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chsn

\brief
	CharacterMovementComponent is an ECS component-system pair which controls character movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"
#include "Game/GrabbableItem.h"

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

	EntityReference hitDebugObject;
	float moveSpeed;
	float rotateSpeed;
	// Not serialized
	EntityReference heldItem;

	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	CharacterMovementComponent();

	const Vec2 GetMovementVector();
	void SetMovementVector(Vec2 vector);

	void DropItem();
	void GrabItem(ecs::CompHandle<GrabbableItemComponent> item);
	void Attack();

	void Serialize(Serializer& writer) const;
	void Deserialize(Deserializer& reader);

	property_vtable()

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw();
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