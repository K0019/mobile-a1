/******************************************************************************/
/*!
\file   UIScreenManager.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/09/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a management system for which UI screen is shown
  at a time.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "EntityUID.h"

/*****************************************************************//*!
\class UIScreenManagerComponent
\brief
	Contains references to UI screen entities and manages whether they
	are active or inactive.
*//******************************************************************/
class UIScreenManagerComponent
	: public IRegisteredComponent<UIScreenManagerComponent>
#ifdef IMGUI_ENABLED
	, public IEditorComponent<UIScreenManagerComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	UIScreenManagerComponent();

	/*****************************************************************//*!
	\brief
		Switches the active screen to the specified screen.
	\param name
		The name of the UI screen entity.
	*//******************************************************************/
	void SwitchToScreen(const std::string& name);

	/*****************************************************************//*!
	\brief
		Switches the previous screen.
	*//******************************************************************/
	void SwitchToPrevScreen();

#ifdef IMGUI_ENABLED
private:
	/*****************************************************************//*!
	\brief
		Draws this component to the inspector.
	\param comp
		The component.
	*//******************************************************************/
	static void EditorDraw(UIScreenManagerComponent& comp);
#endif

public:
	/*****************************************************************//*!
	\brief
		Serializes this component to file.
	\param writer
		The serializer.
	*//******************************************************************/
	void Serialize(Serializer& writer) const override;

	/*****************************************************************//*!
	\brief
		Deserializes this component from file.
	\param reader
		The deserializer.
	*//******************************************************************/
	void Deserialize(Deserializer& reader) override;

private:
	//! The UI screen entities registered.
	std::vector<EntityReference> screens;
	//! The current active screen.
	ecs::EntityHandle currScreen;
	//! The previously active screen.
	ecs::EntityHandle prevScreen;

public:
	property_vtable()

};
property_begin(UIScreenManagerComponent)
{
}
property_vend_h(UIScreenManagerComponent)
