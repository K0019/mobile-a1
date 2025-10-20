/******************************************************************************/
/*!
\file   PlayerMovementComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	PlayerMovementComponent is an ECS component-system pair which takes control of 
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

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
	EntityReference cameraEntity;
	EntityReference playerEntity;

	float minX;
	float maxX;
	float minY;
	float maxY;
	float offsetAmount;
	float offsetDuration;
	float offsetAmountCurrent;	// Not serialised

	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	PlayerMovementComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	~PlayerMovementComponent();

	/*****************************************************************//*!
	\brief
		Offset setter function
	\param offset
		Float value.
	*//******************************************************************/
	void SetOffsetCurrent(float offset);

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw();

	property_vtable()
};
property_begin(PlayerMovementComponent)
{
	property_var(cameraEntity),
	property_var(playerEntity),
	property_var(minX),
	property_var(maxX),
	property_var(minY),
	property_var(maxY),
	property_var(offsetAmount),
	property_var(offsetDuration),
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
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	PlayerMovementComponentSystem();

	/*****************************************************************//*!
	\brief
		Subscribes to relevant messages.
	*//******************************************************************/
	void OnAdded() override;

	/*****************************************************************//*!
	\brief
		Unsubscribes from relevant messages.
	*//******************************************************************/
	void OnRemoved() override;

	/*****************************************************************//*!
	\brief
		Static callback function.
	*//******************************************************************/
	static void OnWaveStarted();

private:
	/*****************************************************************//*!
	\brief
		Updates PlayerMovementComponent.
	\param comp
		The PlayerMovementComponent to update.
	*//******************************************************************/
	void UpdatePlayerMovementComponent(PlayerMovementComponent& comp);
};