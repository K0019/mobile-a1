/******************************************************************************/
/*!
\file   Editor.cpp
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

#include "Editor/Editor.h"
#include "Engine/Events/EventsTypeEditor.h"

#include "Editor/EditorHistory.h"

namespace editor {

	EditorSystem::EditorSystem()
		: selectedEntity{}
	{
	}

	void EditorSystem::OnAdded()
	{
		provider_SelectedEntity = ST<EventsQueue>::Get()->AddEventHandlerFunc<Getters::EditorSelectedEntity>([this](const Getters::EditorSelectedEntity&) -> ecs::EntityHandle {
			return selectedEntity;
		});
	}

	bool EditorSystem::PreRun()
	{
		UpdateSelectedEntity();

		return false;
	}

	void EditorSystem::UpdateSelectedEntity()
	{
		// For each EditorSelectEntity event, set the selected entity to the provided entity
		EventsReader<Events::EditorSelectEntity>::ForEach([this](const Events::EditorSelectEntity& event) -> void {
			selectedEntity = event.entity;
		});

		// Ensure the selected entity is valid
		if (selectedEntity && !ecs::IsEntityHandleValid(selectedEntity))
			selectedEntity = nullptr;

		// Delete selected entity
		EventsReader<Events::EditorDeleteSelectedEntity>::ForEach([this](const Events::EditorDeleteSelectedEntity&) -> void {
			if (!selectedEntity)
				return;
			ST<History>::Get()->OneEvent(HistoryEvent_EntityDelete{ selectedEntity });
			ecs::DeleteEntity(selectedEntity);
			selectedEntity = nullptr;
		});

		// Clone selected entity
		EventsReader<Events::EditorCloneSelectedEntity>::ForEach([this](const Events::EditorCloneSelectedEntity&) -> void {
			if (!selectedEntity)
				return;
			selectedEntity = ecs::CloneEntityNow(selectedEntity, true);
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
		});

		// Create and select entity
		EventsReader<Events::EditorCreateEntityAndSelect>::ForEach([this](const Events::EditorCreateEntityAndSelect&) -> void {
			selectedEntity = ecs::CreateEntity();
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
		});
	}

}
