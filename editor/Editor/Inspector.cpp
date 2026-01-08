/******************************************************************************/
/*!
\file   Inspector.cpp
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

#include "Editor/Inspector.h"
#include "Editor/EditorHistory.h"
#include "ECS/IEditorComponent.h"

#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"

#include "UI/RectTransform.h"
#include "Components/NameComponent.h"
#include "Engine/EntityLayers.h"
#include "Engine/PrefabManager.h"

#include "Utilities/AndroidMacros.h"

namespace editor {

	//for imgui gizmo

	Inspector::Inspector()
		: WindowBase{ ICON_FA_MAGNIFYING_GLASS" Inspector", gui::Vec2{ 300, 400 }, gui::FLAG_WINDOW::ALWAYS_VERTICAL_SCROLL_BAR }
	{
	}

	void Inspector::DrawContainer(int id)
	{
		// Window setup with proper sizing/scaling
		gui::SetNextWindowSizeConstraints(gui::Vec2{ 300, 400 }, gui::Vec2{ FLT_MAX, FLT_MAX });
		Window::DrawContainer(id);
	}

	void Inspector::DrawWindow()
	{
		// Get the selected entity from the editor system
		ecs::EntityHandle selectedEntity{ ST<EventsQueue>::Get()->RequestValueFromEventHandlers<ecs::EntityHandle>(Getters::EditorSelectedEntity{}).value_or(nullptr) };

		// New Entity button - styled and full width
		{
			gui::SetStyleColor buttonCol{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.4f, 0.8f, 1.0f } };
			gui::SetStyleColor buttonHoveredCol{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.3f, 0.5f, 0.9f, 1.0f } };
			if (gui::Button button{ "New Entity", gui::Vec2{ -1.0f, 30.0f } })
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorCreateEntityAndSelect{});
		}

		gui::Separator();

		if(!selectedEntity)
		{
			// No entity selected message
			gui::TextColored(gui::Vec4{ 1.0f, 0.65f, 0.0f, 1.0f }, "No entity selected");
			gui::TextColored(gui::Vec4{ 1.0f, 0.65f, 0.0f, 1.0f }, "Left Click on an Entity to select it");
			return;
		}

		// Entity Header with improved styling
		DrawEntityHeader(selectedEntity);
		// Entity active button
		bool isActive = ecs::GetCompActive(selectedEntity->GetComp<NameComponent>());

		if(isActive) {
			gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4(0.2f, 0.7f, 0.2f, 1.0f) };
			gui::SetStyleColor buttonHoverColor{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4(0.3f, 0.8f, 0.3f, 1.0f) };
			gui::SetStyleColor buttonActiveColor{ gui::FLAG_STYLE_COLOR::BUTTON_ACTIVE, gui::Vec4(0.1f, 0.6f, 0.1f, 1.0f) };
			if(gui::Button("Active")) {
				selectedEntity->SetActive(false);
			}
		}
		else {
			gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4(0.7f, 0.2f, 0.2f, 1.0f) };
			gui::SetStyleColor buttonHoverColor{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4(0.8f, 0.3f, 0.3f, 1.0f) };
			gui::SetStyleColor buttonActiveColor{ gui::FLAG_STYLE_COLOR::BUTTON_ACTIVE, gui::Vec4(0.6f, 0.1f, 0.1f, 1.0f) };
			if(gui::Button("Inactive")) {
				selectedEntity->SetActive(true);
			}
		}

		// Click off button
		{
			gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 0.5f, 0.5f, 0.5f, 0.8f } };
			if(gui::Button button{ "Click here, or double click off to deselect" })
			{
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorSelectEntity{ nullptr });
				return;
			}
		}

		// Transform section header
		{
			gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 0.9f, 0.9f, 0.9f, 1.0f } };
			gui::TextFormatted("Transform: %s", (selectedEntity->GetTransform().GetParent() ? "Local" : "World"));
		}

		// Transform panel
		if (auto rectTransform{ selectedEntity->GetComp<RectTransformComponent>() })
			rectTransform->EditorDraw();
		else
			selectedEntity->GetTransform().EditorDraw();

		gui::Separator();

		// Entity components
		DrawEntityComps(selectedEntity);

		gui::Separator();

		// Add Component section - Redesigned to be more prominent
		DrawAddComp(selectedEntity);

		gui::Separator();

		// Entity action buttons
		DrawEntityActionsButton(selectedEntity);
	}

	void Inspector::DrawEntityHeader(ecs::EntityHandle selectedEntity)
	{
		ecs::CompHandle<NameComponent> nameComp = selectedEntity->GetComp<NameComponent>();
		if(!nameComp)
			return;

		// Create a group for consistent spacing
		gui::Group group{};

		// Style setup
		gui::SetStyleVar styleVarFramePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 8, 3 } };
		gui::SetStyleVar styleVarItemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 4, 0 } };

		// Background and frame styling
		gui::SetStyleColor styleColFrameBg{ gui::FLAG_STYLE_COLOR::FRAME_BG, gui::Vec4{ 0.15f, 0.15f, 0.15f, 1.0f } };
		gui::SetStyleColor styleColFrameBgHovered{ gui::FLAG_STYLE_COLOR::FRAME_BG_HOVERED, gui::Vec4{ 0.2f, 0.2f, 0.2f, 1.0f } };
		gui::SetStyleColor styleColFrameBgActive{ gui::FLAG_STYLE_COLOR::FRAME_BG_ACTIVE, gui::Vec4{ 0.25f, 0.25f, 0.25f, 1.0f } };

		// Label "Name"
		gui::AlignTextToFramePadding();
		gui::TextUnformatted("Name");
		gui::SameLine();

		// Calculate item widths
		const float nameFieldWidth{ gui::GetAvailableContentRegion().x * 0.5f };

		// Input field
		static gui::TextBoxWithBuffer<256> nameInputBox{ "##EntityName" };
		nameInputBox.SetBuffer(nameComp->GetName());

		gui::SetStyleColor styleColText(gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 0.9f, 0.9f, 0.9f, 1.0f });
		gui::SetNextItemWidth(nameFieldWidth);
		if(nameInputBox.Draw())
		{
			std::string newName{ nameInputBox.GetBuffer() };
			if(newName.empty())
				newName = "New Entity";
			nameComp->SetName(newName);
		}

		// Layer field
		ecs::CompHandle<EntityLayerComponent> layerComp{ selectedEntity->GetComp<EntityLayerComponent>() };
		ENTITY_LAYER currLayer{ layerComp->GetLayer() };
		gui::SameLine();
		gui::TextUnformatted("Layer");
		gui::SameLine();

		if(gui::Combo entityLayerCombo{ "##EntityLayer", EntityLayerComponent::GetLayerName(currLayer) })
			for(ENTITY_LAYER i{}; i < ENTITY_LAYER::TOTAL; ++i)
				if(entityLayerCombo.Selectable(EntityLayerComponent::GetLayerName(i), currLayer == i))
					layerComp->SetLayer(i);
	}

	void Inspector::DrawEntityComps(ecs::EntityHandle selectedEntity)
	{
		gui::CollapsingHeader outerHeader{ "Components", gui::FLAG_TREE_NODE::DEFAULT_OPEN };
		if(!outerHeader)
			// The components header isn't opened.
			return;

		// Sort components alphabetically
		std::vector<ecs::EntityCompsIterator> components{};
		for(ecs::EntityCompsIterator compIter{ selectedEntity->Comps_Begin() }, endIter{ selectedEntity->Comps_End() }; compIter != endIter; ++compIter)
			if(!HiddenComponentsStore::IsHidden(compIter.GetCompHash()))
				components.push_back(compIter);
		std::sort(components.begin(), components.end(), [](ecs::EntityCompsIterator& a, ecs::EntityCompsIterator& b) {
			auto lhsMeta = ecs::GetCompMeta(a.GetCompHash());
			auto rhsMeta = ecs::GetCompMeta(b.GetCompHash());
			return lhsMeta->name < rhsMeta->name;
		});

		// Draw the components
		for(int i{}; static_cast<size_t>(i) < components.size(); ++i)
		{
			auto& compIter = components[i];
			const auto compMeta = ecs::GetCompMeta(compIter.GetCompHash());

			gui::SetID id{ i };

			// Draw this component's header
			gui::SetStyleColor styleColHeader{ gui::FLAG_STYLE_COLOR::HEADER, gui::Vec4{ 0.2f, 0.2f, 0.2f, 0.5f } };
			gui::SetStyleColor styleColHeaderHovered{ gui::FLAG_STYLE_COLOR::HEADER_HOVERED, gui::Vec4{ 0.3f, 0.3f, 0.3f, 0.5f } };
			gui::CollapsingHeader innerHeader{ util::AddSpaceBeforeEachCapital(compMeta->name) };
			if(!innerHeader)
				continue; // This component's header isn't opened.

			// Draw component contents
			{
				gui::Indent indent{ 16.0f };
				ST<editor::ComponentDrawMethods>::Get()->Draw(compIter.GetCompHash(), compIter.GetComp<void>());
			}

			// Draw delete button
			bool& deleteConfirmation{ gui::GetVar(("editor_DeleteConfirm_" + std::to_string(compIter.GetCompHash())).c_str(), false) };
			gui::SetStyleColor styleColButton{ gui::FLAG_STYLE_COLOR::BUTTON, (deleteConfirmation ? gui::Vec4{ 0.1f, 0.8f, 0.1f, 1.0f } : gui::Vec4{ 0.8f, 0.1f, 0.1f, 1.0f }) };
			gui::SetStyleColor styleColButtonHovered{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, (deleteConfirmation ? gui::Vec4{ 0.1f, 1.0f, 0.1f, 1.0f } : gui::Vec4{ 1.0f, 0.1f, 0.1f, 1.0f }) };
			if(!deleteConfirmation)
			{
				if(gui::Button deleteButton{ "Delete Component", gui::Vec2{ -1.0f, 0.0f } })
					deleteConfirmation = true;
			}
			else
			{
				const gui::Vec2 buttonSize{ gui::GetAvailableContentRegion().x * 0.5f - 4.0f, 0.0f };
				if(gui::Button deleteButton{ "Confirm Delete", buttonSize })
				{
					RegisteredComponents::GetData(compIter.GetCompHash())->SaveHistory_CompRemove(selectedEntity, compIter.GetComp<void>());
					selectedEntity->RemoveCompByHashNow(compIter.GetCompHash());
					deleteConfirmation = false;
				}

				gui::UnsetStyleColor styleColUnsetButton{ 2 };
				gui::SameLine();
				if(gui::Button cancelButton{ "Cancel", buttonSize })
					deleteConfirmation = false;
			}
		}
	}

	void Inspector::DrawAddComp(ecs::EntityHandle selectedEntity)
	{
		gui::Spacing();
		gui::SetStyleColor styleColFrameBg{ gui::FLAG_STYLE_COLOR::FRAME_BG, gui::Vec4{ 0.2f, 0.2f, 0.2f, 1.0f } };
		gui::SetStyleColor styleColFrameBgHovered{ gui::FLAG_STYLE_COLOR::FRAME_BG_HOVERED, gui::Vec4{ 0.3f, 0.3f, 0.3f, 1.0f } };

		// Static variables for managing popup state
		static gui::TextBoxWithFilter textInput{ true };
		static size_t selectedIndex{};

		// Create a button that visually resembles a combo box
		{
			const float buttonWidth{ gui::GetAvailableContentRegion().x };

			// Draw the main button with text aligned to the left
			gui::SetStyleVar framePaddingStyle{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 8.0f, 6.0f } }; // Match combo padding
			gui::SetStyleVar buttonTextAlignStyle{ gui::FLAG_STYLE_VAR::BUTTON_TEXT_ALIGN, gui::Vec2{ 0.0f, 0.5f } }; // Left-align text

			// Use same colors as combo boxes
			gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::GetStyleColor(gui::FLAG_STYLE_COLOR::FRAME_BG) };
			gui::SetStyleColor buttonHoveredColor{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::GetStyleColor(gui::FLAG_STYLE_COLOR::FRAME_BG_HOVERED) };
			gui::SetStyleColor buttonActiveColor{ gui::FLAG_STYLE_COLOR::BUTTON_ACTIVE, gui::GetStyleColor(gui::FLAG_STYLE_COLOR::FRAME_BG_ACTIVE) };

			// Create the button
			gui::SetNextItemWidth(buttonWidth);
			if(gui::Button button{ "Add Component", gui::Vec2{ buttonWidth, 0.0f } })
			{
				gui::Popup::Open("ComponentPopup");
				textInput.Clear();
				selectedIndex = 0;
			}

			// Draw a small triangle pointing downward (dropdown arrow)
			const gui::Vec2 prevItemRectSize{ gui::GetPrevItemRectSize() };
			const gui::Vec2 prevItemRectMin{ gui::GetPrevItemRectMin() };
			const gui::Vec2 arrowPos{ prevItemRectMin.x + prevItemRectSize.x - 18.0f, prevItemRectMin.y + prevItemRectSize.y / 2.0f };

			gui::DrawTriangle(
				arrowPos + gui::Vec2{ 0.0f, -2.0f },
				arrowPos + gui::Vec2{ 8.0f, -2.0f },
				arrowPos + gui::Vec2{ 4.0f, 4.0f },
				gui::GetStyleColor(gui::FLAG_STYLE_COLOR::TEXT)
			);
		}

		// Calculate popup size and position
		gui::Vec2 popupSize{ gui::GetPrevItemRectSize().x, 300.0f }; // Height can be adjusted
		gui::Vec2 popupPos{ gui::GetPrevItemRectMin() };
		popupPos.y += gui::GetPrevItemRectSize().y;

		// Set next window position and size
		gui::SetNextWindowPos(popupPos);
		gui::SetNextWindowSize(popupSize);

		// Create the popup
		if(gui::Popup popup{ "ComponentPopup", gui::FLAG_WINDOW::NO_MOVE | gui::FLAG_WINDOW::NO_RESIZE })
		{
			// Focus search on open
			gui::SetKeyboardFocusHere();

			// Check selection input control by keyboard
			if(gui::IsKeyPressed(gui::KEY::UP))
				--selectedIndex;
			if(gui::IsKeyPressed(gui::KEY::DOWN))
				++selectedIndex;
			const bool selectedByKeyboard{ gui::IsKeyPressed(gui::KEY::ENTER) };
			const bool selectedByMouse{ gui::IsMouseClicked(gui::MOUSE_BUTTON::LEFT) };
			bool anyItemHovered = false;

			// Search input (fixed at top)
			gui::SetNextItemWidth(gui::GetAvailableContentRegion().x);
			textInput.Draw("##ComponentSearch");
			gui::Separator();

			// Create scrollable region for components
			gui::Child scrollRegionChild{ "ComponentsScrollRegion", gui::Vec2{ 0.0f, gui::GetAvailableContentRegion().y } };

			// Get filtered components
			auto sortedComponents{ util::ToSortedVectorOfRefs<ecs::CompHash, RegisteredComponentData>(
					RegisteredComponents::Begin(), RegisteredComponents::End(),
				// Sort based on component name.
				[](const auto& a, const auto& b) -> bool {
						return a.second.get().name < b.second.get().name;
				},
				// Select only components that are not editor hidden, not already attached to the entity,
				// and match the search string (if any)
				[entity = selectedEntity](const auto& a) -> bool {
					if(a.second.isEditorHidden || entity->HasComp(a.first))
						return false;
					return textInput.PassFilter(a.second.name.c_str());
				}
			) };

			// Ensure selected index is within range
			if(selectedIndex >= sortedComponents.size())
				selectedIndex = 0;

			for(size_t index{}; index < sortedComponents.size(); ++index)
			{
				const RegisteredComponentData& registeredData{ sortedComponents[index].second.get() };
				bool isSelected = (index == selectedIndex);

				// TRACKING STATE IN OUR APPLICATION BREAKS IMGUI'S CLICK LOGIC
				gui::Selectable(registeredData.name.c_str(), isSelected);

				// use hovering logic to track if the mouse is within the popup or not, because otherwise only tracking if left mouse button is clicked causes it to work no matter where the mouse is.
				bool thisItemHovered = gui::IsItemHovered();
				if(thisItemHovered) {
					anyItemHovered = true;
					selectedIndex = index;
				}

				// Item is selected if it's hovered AND mouse is clicked, OR keyboard selected
				if((thisItemHovered && selectedByMouse) || (selectedByKeyboard && isSelected))
				{
					// Attach the component
					registeredData.ConstructDefaultAndAttachTo(selectedEntity);
					registeredData.SaveHistory_CompAdd(selectedEntity);
					textInput.Clear();
					popup.Close();
				}
			}

			// If mouse was clicked but no item was hovered, close the popup
			if(selectedByMouse && !anyItemHovered) {
				popup.Close();
			}
		}

		gui::Spacing();
	}
	void Inspector::DrawEntityActionsButton(ecs::EntityHandle selectedEntity)
	{
		// Clone button
		{
			gui::SetStyleColor styleColButton{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.5f, 0.7f, 1.0f } };
			gui::SetStyleColor styleColButtonHovered{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.3f, 0.6f, 0.8f, 1.0f } };
			if (gui::Button cloneButton{ "Clone Entity", gui::Vec2{ -1.0f, 30.0f } })
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorCloneSelectedEntity{});
			if(gui::Button savePrefabButton{ "Save as Prefab", gui::Vec2{ -1.0f, 30.0f } })
				PrefabManager::SavePrefab(selectedEntity, selectedEntity->GetComp<NameComponent>()->GetName());
		}

		gui::Spacing();

		// Delete entity with confirmation
		bool& deleteConfirmation{ gui::GetVar("editor_DeleteConfirmation", false) };
		gui::SetStyleColor styleColButton{ gui::FLAG_STYLE_COLOR::BUTTON, (deleteConfirmation ? gui::Vec4{ 0.1f, 0.8f, 0.1f, 1.0f } : gui::Vec4{ 0.8f, 0.1f, 0.1f, 1.0f }) };
		gui::SetStyleColor styleColButtonHovered{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, (deleteConfirmation ? gui::Vec4{ 0.1f, 1.0f, 0.1f, 1.0f } : gui::Vec4{ 1.0f, 0.1f, 0.1f, 1.0f }) };

		if(!deleteConfirmation)
		{
			if(gui::Button deleteButton{ "Delete Entity", gui::Vec2{ -1.0f, 30.0f } })
				deleteConfirmation = true;
		}
		else
		{
			const gui::Vec2 buttonSize{ gui::GetAvailableContentRegion().x * 0.5f - 4.0f, 30.0f };
			if(gui::Button confirmButton{ "Confirm Delete", buttonSize })
			{
				ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorDeleteSelectedEntity{});
				deleteConfirmation = false;
			}

			gui::UnsetStyleColor styleColUnsetButton{ 2 };
			gui::SameLine();
			if(gui::Button cancelButton{ "Cancel", buttonSize })
				deleteConfirmation = false;
		}
	}

}