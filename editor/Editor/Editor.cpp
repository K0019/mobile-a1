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

#include "Engine/Input.h"
#include "Editor/EditorHistory.h"
#include "Editor/EditorGizmoBridge.h"
#include "Editor/MaterialCreation.h"

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

		return false; // No components to run
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
			selectedEntity = ecs::CloneEntity(selectedEntity, true);
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
		});

		// Create and select entity
		EventsReader<Events::EditorCreateEntityAndSelect>::ForEach([this](const Events::EditorCreateEntityAndSelect&) -> void {
			selectedEntity = ecs::CreateEntity();
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
		});

		// Open material editor
		EventsReader<Events::OpenMaterialEditor>::ForEach([](const Events::OpenMaterialEditor& event) -> void {
			CreateGuiWindow<MaterialEditWindow>(event.materialHash);
		});
	}

	bool EditorInputSystem::PreRun()
	{
#ifdef IMGUI_ENABLED
		if (ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::LCTRL))
		{
			if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::Z))
				ST<History>::Get()->UndoOne();
			else if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::Y))
				ST<History>::Get()->RedoOne();
		}

		if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::DEL))
			ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorDeleteSelectedEntity{});

		if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F1)) {
			EditorGizmo_Publish(ImGuizmo::TRANSLATE, ImGuizmo::WORLD /* or LOCAL */);
		}
		if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F2)) {
			EditorGizmo_Publish(ImGuizmo::ROTATE, EditorGizmo_Mode());
		}
		if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F3)) {
			EditorGizmo_Publish(ImGuizmo::SCALE, EditorGizmo_Mode());
		}
#endif

		return false; // No components to run
	}

}
