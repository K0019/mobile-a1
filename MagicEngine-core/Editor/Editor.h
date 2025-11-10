/******************************************************************************/
/*!
\file   Editor.h
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
#include "Engine/Events/EventsQueue.h"

namespace editor {

	class EditorSystem : public ecs::System<EditorSystem>
	{
	public:
		// Initialize default values to variables
		EditorSystem();

		// The initialize function
		void OnAdded() override;

		// The Update function
		bool PreRun() override;

	private:
		void UpdateSelectedEntity();

	private:
		AutoEventHandler provider_SelectedEntity;

		ecs::EntityHandle selectedEntity;

	};

}
