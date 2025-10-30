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
	GrabbableItemComponent is an ECS component-system pair which controls player movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

/*****************************************************************//*!
\class GrabbableItemComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class GrabbableItemComponent
	: public IRegisteredComponent<GrabbableItemComponent>
	, public IEditorComponent<GrabbableItemComponent>
{
public:
	void Attack(Vec3 origin, Vec3 direction);
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	GrabbableItemComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	//~GrabbableItemComponent();

	// Not serialized
	bool isHeld;
	EntityReference owner;

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

property_begin(GrabbableItemComponent)
{
}
property_vend_h(GrabbableItemComponent)

/*****************************************************************//*!
\class GrabbableItemComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class GrabbableItemComponentSystem : public ecs::System<GrabbableItemComponentSystem, GrabbableItemComponent>
{
public:
	GrabbableItemComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates GrabbableItemComponent.
	\param comp
		The GrabbableItemComponent to update.
	*//******************************************************************/
	void UpdateGrabbableItemComponent(GrabbableItemComponent& comp);
};