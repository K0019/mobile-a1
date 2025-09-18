/******************************************************************************/
/*!
\file   NameComponent.h
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

/*****************************************************************//*!
\class NameComponent
\brief
	Attaches a name string to help identify entities.
*//******************************************************************/
class NameComponent : public IRegisteredComponent<NameComponent>, IHiddenComponent<NameComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructs this component with the name "New Entity"
	*//******************************************************************/
	NameComponent();

	/*****************************************************************//*!
	\brief
		Constructs this component with the provided name.
	\param name
		The name of the entity.
	*//******************************************************************/
	NameComponent(const std::string& name);

	/*****************************************************************//*!
	\brief
		Gets the stored name.
	\return
		The entity's name.
	*//******************************************************************/
	const std::string& GetName() const;

	/*****************************************************************//*!
	\brief
		Sets the stored name.
	\param newName
		The entity's new name.
	*//******************************************************************/
	void SetName(const std::string& newName);

private:
	//! The name of this entity.
	std::string name;

	property_vtable()
};
property_begin(NameComponent)
{
	property_var(name)
}
property_vend_h(NameComponent)

namespace util { namespace name {

	/*****************************************************************//*!
	\brief
		Gets the first encountered child entity with the specified name.
	\param parent
		The parent entity.
	\param name
		The name to find.
	\return
		The first found entity with the specified name. If none exist, nullptr.
	*//******************************************************************/
	ecs::EntityHandle GetEntityWithNameInChildren(ecs::EntityHandle parent, const std::string& name);
	/*****************************************************************//*!
	\brief
		Gets the first encountered child entity with the specified name.
	\param parent
		The parent entity.
	\param name
		The name to find.
	\return
		The first found entity with the specified name. If none exist, nullptr.
	*//******************************************************************/
	ecs::ConstEntityHandle GetEntityWithNameInChildren(ecs::ConstEntityHandle parent, const std::string& name);

} }
