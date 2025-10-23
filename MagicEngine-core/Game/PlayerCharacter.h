/******************************************************************************/
/*!
\file   PlayerCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	PlayerMovementComponent is an ECS component-system pair which controls player movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

/*****************************************************************//*!
\class PlayerMovementComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class PlayerMovementComponent
	: public IRegisteredComponent<PlayerMovementComponent>
	, public IEditorComponent<PlayerMovementComponent>
{
public:
	float moveSpeed;
	float rotateSpeed;
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	PlayerMovementComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	//~PlayerMovementComponent();


	property_vtable()

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw();

};

property_begin(PlayerMovementComponent)
{
	property_var(moveSpeed),
	property_var(rotateSpeed)
}
property_vend_h(PlayerMovementComponent)

/*****************************************************************//*!
\class PlayerMovementComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class PlayerMovementComponentSystem : public ecs::System<PlayerMovementComponentSystem, PlayerMovementComponent>
{
public:
	PlayerMovementComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates PlayerMovementComponent.
	\param comp
		The PlayerMovementComponent to update.
	*//******************************************************************/
	void UpdatePlayerMovementComponent(PlayerMovementComponent& comp);
};