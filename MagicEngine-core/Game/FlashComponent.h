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
#include "ECS/ECS.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

/*****************************************************************//*!
\class FlashComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class FlashComponent
	: public IRegisteredComponent<FlashComponent>
	, public IEditorComponent<FlashComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	FlashComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	//~FlashComponent();

	// Serialized
	float flashTime;
	UserResourceHandle<ResourceMaterial> flashMaterial;

	property_vtable()

	// Not serialized
	float currentFlashTime;
	bool isFlashing;
public:
	void Flash();

	std::vector<UserResourceHandle<ResourceMaterial>> defaultMaterials;
private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

};

property_begin(FlashComponent)
{
	property_var(flashTime),
	property_var(flashMaterial)
}
property_vend_h(FlashComponent)

/*****************************************************************//*!
\class FlashComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class FlashComponentSystem : public ecs::System<FlashComponentSystem, FlashComponent>
{
public:
	FlashComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates FlashComponent.
	\param comp
		The FlashComponent to update.
	*//******************************************************************/
	void UpdateFlashComponent(FlashComponent& comp);
};