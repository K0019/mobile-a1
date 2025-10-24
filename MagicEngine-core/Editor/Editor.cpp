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
#include "Editor/EditorHistory.h"
#include "Engine/Input.h"
#include "Editor/IEditorComponent.h"

#include "Engine/Engine.h"
#include "Graphics/TextComponent.h"

#include "Components/NameComponent.h"
#include "Engine/EntityLayers.h"
#include "Game/GameSystems.h"
#include "Editor/EditorGizmoBridge.h"
#include "Engine/PrefabManager.h"


#ifdef IMGUI_ENABLED

//for imgui gizmo

Inspector::Inspector()
	: Window{ ICON_FA_MAGNIFYING_GLASS" Inspector", gui::Vec2{ 300, 400 }, gui::FLAG_WINDOW::ALWAYS_VERTICAL_SCROLL_BAR }
{
}

Inspector::~Inspector()
{
	ST<History>::Destroy();
}

void Inspector::ProcessInput()
{
	if(ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::LCTRL))
		if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::Z))
			ST<History>::Get()->UndoOne();
		else if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::Y))
			ST<History>::Get()->RedoOne();

	// Undoing or somewhere outside may have deleted the selected entity. Check if it still exists. If not, deselect it.
	CheckIsSelectedEntityValid();

	if(ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::DEL))
		DeleteSelectedEntity();

	//to change guizmo settings
	if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F1)) {
		EditorGizmo_Publish(ImGuizmo::TRANSLATE, ImGuizmo::WORLD /* or LOCAL */);
	}
	if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F2)) {
		EditorGizmo_Publish(ImGuizmo::ROTATE, EditorGizmo_Mode());
	}
	if (ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F3)) {
		EditorGizmo_Publish(ImGuizmo::SCALE, EditorGizmo_Mode());
	}
}

void Inspector::DrawContainer(int id)
{
	// Window setup with proper sizing/scaling
	gui::SetNextWindowSizeConstraints(gui::Vec2{ 300, 400 }, gui::Vec2{ FLT_MAX, FLT_MAX });
	Window::DrawContainer(id);
}

void Inspector::DrawContents()
{
	// If the selected entity doesn't exist anymore, deselect it
	CheckIsSelectedEntityValid();

	// Settings section
	if(gui::CollapsingHeader header{ "Settings", gui::FLAG_TREE_NODE::DEFAULT_OPEN })
	{
		gui::Checkbox("Draw Selection Boxes", &drawBoxes);
	}

	// New Entity button - styled and full width
	{
		gui::SetStyleColor buttonCol{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.4f, 0.8f, 1.0f } };
		gui::SetStyleColor buttonHoveredCol{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.3f, 0.5f, 0.9f, 1.0f } };
		if(gui::Button button{ "New Entity", gui::Vec2{ -1.0f, 30.0f } })
			CreateEntityAndSelect();
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
	DrawEntityHeader();
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
			SetSelectedEntity(nullptr);
			return;
		}
	}

	// Transform section header
	{
		gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 0.9f, 0.9f, 0.9f, 1.0f } };
		gui::TextFormatted("Transform: %s", (selectedEntity->GetTransform().GetParent() ? "Local" : "World"));
	}

	// Gizmo tool buttons
	{
		ImGuizmo::Enable(true);
	}


	// Transform panel
	selectedEntity->GetTransform().EditorDraw();

	gui::Separator();

	// Entity components
	DrawEntityComps();

	gui::Separator();

	// Add Component section - Redesigned to be more prominent
	DrawAddComp();

	gui::Separator();

	// Entity action buttons
	DrawEntityActionsButton();
}

void Inspector::CreateEntityAndSelect()
{
	SetSelectedEntity(ecs::CreateEntity());
	ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
	selectedEntity->GetTransform().SetLocal(Vec3{ 200.0f, 200.0f, 0.0f }, Vec3{ 150.0f, 150.0f, 150.0f }, Vec3{});
	//selectedEntity->AddCompNow(RenderComponent{});
}

void Inspector::ForceUnselectEntity()
{
	SetSelectedEntity(nullptr);
}

void Inspector::DrawSceneView()
{
	ImGui::Separator();
	ImGui::Text("Physics Settings");
	ImGui::Checkbox("Show Colliders", &m_drawPhysicsBoxes);
	ImGui::Checkbox("Draw Velocity", &m_drawVelocity);
	ImGui::Separator();
	ImGui::Text("Grid Settings");
	ImGui::Checkbox("Show Grid", &m_showGrid);
	ImGui::Checkbox("Snap to Grid", &m_snapToGrid);
	ImGui::DragFloat("Grid Size", &m_gridSize, 10.0f, 10.0f, 1000.0f);
	//ImGui::DragFloat2("Grid Offset", &m_gridOffset.x, 10.0f);
	//ImGui::ColorEdit3("Grid Color", &m_gridColor.x);
	//VkClearValue clearColor = ST<Engine>::Get()->_vulkan->_renderer->getClearColor();
	/*if(ImGui::ColorEdit3("Background Color", clearColor.color.float32))
	{
		ST<Engine>::Get()->_vulkan->_renderer->setClearColor(clearColor);
	}*/
}

ecs::EntityHandle Inspector::GetSelectedEntity() {
	return selectedEntity;
}
void Inspector::SetSelectedEntity(ecs::EntityHandle entity)
{
	selectedEntity = entity;

}

void Inspector::DrawSelectedEntityBorder()
{
	if(CheckIsSelectedEntityValid() && drawBoxes)
	{
		Transform textTransform;
		const Transform* transform{};
		if(ecs::CompHandle<TextComponent> textComp{ selectedEntity->GetComp<TextComponent>() })
		{
			textTransform = selectedEntity->GetComp<TextComponent>()->GetWorldTextTransform();
			transform = &textTransform;
		}
		else
		{
			transform = &selectedEntity->GetTransform();
		}
		// TODO 3D: Editor, draw entity bounding box
		//util::DrawBoundingBox(*transform, { 0.0f, 1.0f, 0.0f });
	}
}

void Inspector::DeleteSelectedEntity()
{
	if(!selectedEntity)
		return;

	ST<History>::Get()->OneEvent(HistoryEvent_EntityDelete{ selectedEntity });
	ecs::DeleteEntity(selectedEntity);
	SetSelectedEntity(nullptr);
}

bool Inspector::CheckIsSelectedEntityValid()
{
	if(selectedEntity && !ecs::IsEntityHandleValid(selectedEntity))
		selectedEntity = nullptr;
	return selectedEntity;
}

void Inspector::DrawEntityHeader()
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

void Inspector::DrawEntityComps()
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
				selectedEntity->RemoveCompNow(compIter.GetCompHash());
				deleteConfirmation = false;
			}

			gui::UnsetStyleColor styleColUnsetButton{ 2 };
			gui::SameLine();
			if(gui::Button cancelButton{ "Cancel", buttonSize })
				deleteConfirmation = false;
		}
	}
}

void Inspector::DrawAddComp()
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
		const bool selectedByMouse{ ImGui::IsMouseClicked(ImGuiMouseButton_Left) };
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
					if(a.second.isEditorHidden || entity->HasComp(a.first)) {
							return false;
					}

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
			bool thisItemHovered = ImGui::IsItemHovered();
			if(thisItemHovered) {
				anyItemHovered = true;
				selectedIndex = index;
			}

			// Item is selected if it's hovered AND mouse is clicked, OR keyboard selected
			if((thisItemHovered && selectedByMouse) || (selectedByKeyboard && isSelected))
			{
				// Attach the component
				registeredData.ConstructDefaultAndAttachNowTo(selectedEntity);
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
void Inspector::DrawEntityActionsButton()
{
	// Clone button
	{
		gui::SetStyleColor styleColButton{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.5f, 0.7f, 1.0f } };
		gui::SetStyleColor styleColButtonHovered{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.3f, 0.6f, 0.8f, 1.0f } };
		if(gui::Button cloneButton{ "Clone Entity", gui::Vec2{ -1.0f, 30.0f } })
			SetSelectedEntity(ecs::CloneEntityNow(selectedEntity, true));
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
			DeleteSelectedEntity();
			deleteConfirmation = false;
		}

		gui::UnsetStyleColor styleColUnsetButton{ 2 };
		gui::SameLine();
		if(gui::Button cancelButton{ "Cancel", buttonSize })
			deleteConfirmation = false;
	}
}

#endif