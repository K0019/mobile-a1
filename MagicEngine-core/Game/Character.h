/******************************************************************************/
/*!
\file   CharacterCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	CharacterMovementComponent is an ECS component-system pair which controls character movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

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
	float moveSpeed;
	float rotateSpeed;
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	CharacterMovementComponent();

	const Vec2 GetMovementVector();
	void SetMovementVector(Vec2 vector);

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