/******************************************************************************/
/*!
\file   PlayerCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	PlayerCharacter is an ECS component-system pair which controls player movement. 

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

	// Serialized
	EntityReference cameraReference;
	float grabDistance;

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	property_vtable()

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

};

property_begin(PlayerMovementComponent)
{
	property_var(grabDistance)
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