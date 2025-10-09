/******************************************************************************/
/*!
\file   IHiddenComponent.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for IHiddenComponent, which hides ecs components from
  being displayed in the editor inspector window.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

/*****************************************************************//*!
\class IHiddenComponent
\brief
	Hides ecs components that inherit this from being displayed in the
	editor inspector window.
\tparam CompType
	The component type.
*//******************************************************************/
template <typename CompType>
class IHiddenComponent
{
private:
	/*****************************************************************//*!
	\brief
		Registers the component type into HiddenComponentsStore.
	\return
		Dummy bool.
	*//******************************************************************/
	static bool RegisterComponent();

	//! Calls RegisterComponent() once when this static is initialized at startup
	inline static bool isRegistered{ RegisterComponent() };

};

/*****************************************************************//*!
\class HiddenComponentsStore
\brief
	Keeps a list of hidden components in this project.
*//******************************************************************/
class HiddenComponentsStore
{
public:
	/*****************************************************************//*!
	\brief
		Checks whether a component with the given component hash is a
		hidden component.
	\param compHash
		The hash of the component type.
	\return
		True if the component is hidden. False otherwise.
	*//******************************************************************/
	static bool IsHidden(ecs::CompHash compHash);

	/*****************************************************************//*!
	\brief
		Registers a component with the specified component hash as a
		hidden component.
	\param compHash
		The hash of the component type.
	*//******************************************************************/
	static void RegisterComponent(ecs::CompHash compHash);

private:
	//! The list of hidden components.
	std::unordered_set<ecs::CompHash> registeredCompHashes;
};

template<typename CompType>
bool IHiddenComponent<CompType>::RegisterComponent()
{
	HiddenComponentsStore::RegisterComponent(ecs::GetCompHash<CompType>());
	return true;
}
