/******************************************************************************/
/*!
\file   Inspector.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the Editor class.
The Editor class is responsible for processing input and drawing the user interface for the game editor.
It also maintains the state of the editor, such as the selected entity and component.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Editor/Containers/GUIAsECS.h"

namespace editor {

	class Inspector : public WindowBase<Inspector>
	{
#ifdef IMGUI_ENABLED
	public:
		Inspector();

		void DrawContainer(int id) override;

		/**
		* \brief Draws the user interface for the editor.
		*/
		void DrawWindow() override;

	private:
		/*****************************************************************//*!
		\brief
			Draws the selected entity's name and layer.
		*//******************************************************************/
		void DrawEntityHeader(ecs::EntityHandle selectedEntity);

		/*****************************************************************//*!
		\brief
			Draws the selected entity's components.
		*//******************************************************************/
		void DrawEntityComps(ecs::EntityHandle selectedEntity);

		/*****************************************************************//*!
		\brief
			Draws the add component section.
		*//******************************************************************/
		void DrawAddComp(ecs::EntityHandle selectedEntity);

		/*****************************************************************//*!
		\brief
			Draw buttons that interact with the selected entity.
		*//******************************************************************/
		void DrawEntityActionsButton(ecs::EntityHandle selectedEntity);

#endif
	};

}
