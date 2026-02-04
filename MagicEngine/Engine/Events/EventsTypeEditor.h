/******************************************************************************/
/*!
\file   EventsQueue.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\date   10/19/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Provides a buffer to queue events for event handlers to later pull.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "ECS/ECS.h"
#include "Editor/Containers/GUICollection.h"

namespace Events {
	
	// Consumed by: EditorSystem
	struct EditorSelectEntity
	{
		ecs::EntityHandle entity;
	};
	// Consumed by: EditorSystem
	struct EditorCreateEntityAndSelect {};
	// Consumed by: EditorSystem
	struct EditorCloneSelectedEntity {};
	// Consumed by: EditorSystem
	struct EditorDeleteSelectedEntity {};

	// Consumed by: CustomViewport
	struct ResizeViewport
	{
		unsigned int width, height;
	};

	// Consumed by: Popup
	struct PopupOpenRequest
	{
		std::string title;
		std::string message;
		gui::PopupWindow::FLAG flags{ gui::PopupWindow::FLAG::NONE };
	};
	// Consumed by: Popup
	struct PopupCloseRequest {};

	// Consumed by: Editor (MaterialTab or similar)
	struct OpenMaterialEditor
	{
		size_t materialHash;
	};

}

namespace Getters {

	// Provided by: EditorSystem
	struct EditorSelectedEntity {};

}
