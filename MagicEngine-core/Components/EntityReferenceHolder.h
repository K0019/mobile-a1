/******************************************************************************/
/*!
\file   EntityReferenceHolderComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for the name component and an attacher class that
  automatically attaches name components to new entities created.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

/*****************************************************************//*!
\class EntityReferenceHolderComponent
\brief
	Attaches a name string to help identify entities.
*//******************************************************************/
class EntityReferenceHolderComponent 
	: public IRegisteredComponent<EntityReferenceHolderComponent>
	, public IEditorComponent<EntityReferenceHolderComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructs this component with the name "New Entity"
	*//******************************************************************/
	EntityReferenceHolderComponent();

	~EntityReferenceHolderComponent();

	/*****************************************************************//*!
	\brief
		Constructs this component with the provided name.
	\param name
		The name of the entity.
	*//******************************************************************/
	EntityReferenceHolderComponent(const std::string& name);

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	void UpdateVectors();

	void UpdateMap();

private:
	virtual void EditorDraw() override;

	std::unordered_map<std::string, EntityReference> referencedComponents;
	std::vector<char*> entityNames;
	std::vector<EntityReference> entityReferences;
	property_vtable()
};
property_begin(EntityReferenceHolderComponent)
{
}
property_vend_h(EntityReferenceHolderComponent)

