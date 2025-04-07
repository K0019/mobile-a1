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

#include "Editor.h"
#include "EditorHistory.h"

#include "Engine.h"
#include "TextComponent.h"
#include "ryan-c/Renderer.h"
#include "NameComponent.h"
#include "EntityLayers.h"
#include "Game.h"

#ifdef IMGUI_ENABLED

Editor::Editor()
	: Window{ ICON_FA_MAGNIFYING_GLASS" Inspector", gui::Vec2{ 300, 400 }, gui::FLAG_WINDOW::ALWAYS_VERTICAL_SCROLL_BAR }
{
}

Editor::~Editor()
{
	ST<History>::Destroy();
}

void Editor::ProcessInput()
{
	static bool isDragging = false;
	static bool isCameraDragging = false;
	static Vector2 lastMousePos;
	static Vector2 lastCameraMousePos;

	const Vector2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
	const Vector2 worldMousePos = ST<CustomViewport>::Get()->WindowToWorldPosition(mousePos);

	if(Input::GetKeyCurr(KEY::LCTRL))
		if(Input::GetKeyPressed(KEY::Z))
			ST<History>::Get()->UndoOne();
		else if(Input::GetKeyPressed(KEY::Y))
			ST<History>::Get()->RedoOne();

	// Undoing or somewhere outside may have deleted the selected entity. Check if it still exists. If not, deselect it.
	CheckIsSelectedEntityValid();

	bool isMouseInViewport = ST<CustomViewport>::Get()->IsMouseInViewport(mousePos);
	float dt{ GameTime::FixedDt() };
	if(Input::GetKeyCurr(KEY::UP))
	{
		ST<CameraController>::Get()->AddPosition({ 0, 1000.0f * dt });
	}
	else if(Input::GetKeyCurr(KEY::DOWN))
	{
		ST<CameraController>::Get()->AddPosition({ 0, -1000.0f * dt });
	}
	if(Input::GetKeyCurr(KEY::LEFT))
	{
		ST<CameraController>::Get()->AddPosition({ -1000.0f * dt, 0 });
	}
	else if(Input::GetKeyCurr(KEY::RIGHT))
	{
		ST<CameraController>::Get()->AddPosition({ 1000.0f * dt, 0 });
	}

	if(isMouseInViewport && ST<Game>::Get()->GetState() == GAMESTATE::EDITOR) // Only able to scroll camera while in editor mode
	{
		ST<CameraController>::Get()->MultTargetZoom(CameraController::GetZoomMultiplierFromInput(Input::GetScroll(), ST<GameSettings>::Get()->m_editorZoomSensitivity));

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			isCameraDragging = true;
			lastCameraMousePos = mousePos;  // Store screen space coordinates
		}
		else if(ImGui::IsMouseReleased(ImGuiMouseButton_Right))
		{
			isCameraDragging = false;
		}

		if(isCameraDragging && ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			Vector2 mouseDelta = mousePos - lastCameraMousePos;

			// Convert screen space delta to world space delta
			float zoomFactor = ST<CameraController>::Get()->GetZoom();
			Vector2 worldDelta = mouseDelta / zoomFactor;
			worldDelta.x *= -1;  // Invert x axis
			// Apply camera movement
			ST<CameraController>::Get()->AddPosition(worldDelta);

			// Update last mouse position
			lastCameraMousePos = mousePos;////
		}
	}
	else
	{
		isCameraDragging = false;
	}
	ST<CameraController>::Get()->LerpZoom(dt);

	if(Input::GetKeyPressed(KEY::DEL))
		DeleteSelectedEntity();

	if(!selectedEntity)
	{
		if(!isMouseInViewport || isCameraDragging)
		{
			return;
		}
		Transform textTransform; // For storing the transform of the text.
		float zpos = -1;
		for(auto iter = ecs::GetEntitiesBegin(), endIter = ecs::GetEntitiesEnd(); iter != endIter; ++iter)
		{
			const auto entity = *iter;
			/*if(iter->GetComp<SpriteComponent>() && iter->GetComp<SpriteComponent>()->GetTextureHash() == util::GenHash("background"))
			{
				continue;
			}*/
			// There is an issue with the following line of code, where 2 unnecessary copies of Transform are being performed.
			// const Transform& transform = (iter->GetComp<TextComponent>())
			//		? iter->GetComp<TextComponent>()->GetWorldTextTransform()
			//		: entity->GetTransform();
			// It is likely because the compiler is converting GetTransform()'s lvalue ref into an rvalue for the return of ? operator,
			// then copying it into the const lvalue ref of transform.
			// To workaround this, we'll use pointers instead.
			const Transform* transform{};
			if(ecs::CompHandle<TextComponent> textComp{ iter->GetComp<TextComponent>() })
			{
				textTransform = iter->GetComp<TextComponent>()->GetWorldTextTransform();
				transform = &textTransform;
			}
			else
			{
				transform = &entity->GetTransform();
			}
			//DrawBoundingBox(transform);
			/*if(drawBoxes)*/
				//DrawBoundingBox(*transform, Vector3(1.0f, 0.0f, 0.0f));

			if(ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
				 util::IsPointInside(mousePos, ST<CustomViewport>::Get()->WorldToWindowTransform(*transform)) &&
				 ST<Game>::Get()->GetState() == GAMESTATE::EDITOR) // Only able to select through clicking while in editor mode
			{
				if(transform->GetZPos() > zpos)
				{
					zpos = transform->GetZPos();
					SetSelectedEntity(entity);
					ST<CustomViewport>::Get()->SetDisableMoving(true);
				}
			}
		}
	}
	else
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


		if(!isMouseInViewport || isCameraDragging)
		{
			isDragging = false;
			return;
		}

		m_gizmo.processInput();
		// Handle deselection with double click
		static float lastClickTime = 0.0f;
		static const float doubleClickTime = 0.3f; // Adjust this value to change double click sensitivity

		if(ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			float currentTime = static_cast<float>(ImGui::GetTime());
			if(currentTime - lastClickTime < doubleClickTime)
			{
				// Double click detected
				if(!util::IsPointInside(worldMousePos, *transform))
				{
					SetSelectedEntity(nullptr);
					isDragging = false;
					ST<CustomViewport>::Get()->SetDisableMoving(false);
					lastClickTime = 0.0f;
					return;
				}
			}
			lastClickTime = currentTime;
		}

		// Only handle dragging if gizmo is not active
		if(m_currentGizmoType == GizmoType::None)
		{
			if(isDragging) {
				if(ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
					Vector2 newPosition = worldMousePos;
					if(!m_snapToGrid) {
						const Vector2 delta = worldMousePos - lastMousePos;
						newPosition = transform->GetWorldPosition() + delta;
					}
					else
					{
						newPosition = SnapToGrid(newPosition);
					}
					ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ selectedEntity, selectedEntity->GetTransform().GetLocalPosition() });
					selectedEntity->GetTransform().SetWorldPosition(newPosition);
					lastMousePos = worldMousePos;
				}
				else {
					isDragging = false;
					ST<CustomViewport>::Get()->SetDisableMoving(false);
				}
			}
			else {
				if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
					if(util::IsPointInside(worldMousePos, *transform))
					{
						isDragging = true;
						lastMousePos = worldMousePos;
						ST<CustomViewport>::Get()->SetDisableMoving(true);
					}
				}
			}
		}
	}
}

void Editor::DrawContainer()
{
	// Window setup with proper sizing/scaling
	gui::SetNextWindowSizeConstraints(gui::Vec2{ 300, 400 }, gui::Vec2{ FLT_MAX, FLT_MAX });
	Window::DrawContainer();
}

void Editor::DrawContents()
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
	if(gui::Group groupGizmo{})
	{
		const auto RenderGizmoButton{ [&gizmo = m_gizmo, &currType = m_currentGizmoType](GizmoType type, const char* buttonText, const char* hoverText) -> void {
			gui::SetStyleColor styleColButton{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.2f, 0.5f, 0.7f, 1.0f }, currType == type };
			if(gui::Button button{ buttonText })
			{
				currType = (currType == type ? GizmoType::None : type);
				gizmo.setType(currType);
			}
			if(gui::IsItemHovered())
				gui::Tooltip tooltip{ hoverText };
		} };

		RenderGizmoButton(GizmoType::Translate, ICON_FA_ARROWS_LEFT_RIGHT, "Translate");
		gui::SameLine();
		RenderGizmoButton(GizmoType::Rotate, ICON_FA_ROTATE, "Rotate");
		gui::SameLine();
		RenderGizmoButton(GizmoType::Scale, ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER, "Scale");
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

void Editor::CreateEntityAndSelect()
{
	SetSelectedEntity(ecs::CreateEntity());
	ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ selectedEntity });
	selectedEntity->GetTransform().SetLocal(Vector2{ 200.0f, 200.0f }, Vector2{ 150.0f, 150.0f }, 0.0f);
	selectedEntity->AddCompNow(RenderComponent{});
}

void Editor::ForceUnselectEntity()
{
	SetSelectedEntity(nullptr);
}

void Editor::DrawSceneView()
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
	ImGui::DragFloat2("Grid Offset", &m_gridOffset.x, 10.0f);
	ImGui::ColorEdit3("Grid Color", &m_gridColor.x);
	VkClearValue clearColor = ST<Engine>::Get()->_vulkan->_renderer->getClearColor();
	if(ImGui::ColorEdit3("Background Color", clearColor.color.float32))
	{
		ST<Engine>::Get()->_vulkan->_renderer->setClearColor(clearColor);
	}
}

ecs::EntityHandle Editor::GetSelectedEntity() {
	return selectedEntity;
}
void Editor::SetSelectedEntity(ecs::EntityHandle entity)
{
	selectedEntity = entity;
	if(selectedEntity) {
		m_gizmo.attach(selectedEntity->GetTransform());
		m_gizmo.setType(m_currentGizmoType = GizmoType::None);
	}
	else {
		m_gizmo.detach();
		m_gizmo.setType(m_currentGizmoType = GizmoType::None);
	}
}

void Editor::DrawSelectedEntityBorder()
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
		util::DrawBoundingBox(*transform, { 0.0f, 1.0f, 0.0f });
	}
}

void Editor::DrawGizmoInViewport(ImDrawList* drawList)
{
	if(CheckIsSelectedEntityValid() && m_currentGizmoType != GizmoType::None)
	{
		m_gizmo.draw(drawList);
	}
}

void Editor::DeleteSelectedEntity()
{
	if(!selectedEntity)
		return;

	ST<History>::Get()->OneEvent(HistoryEvent_EntityDelete{ selectedEntity });
	ecs::DeleteEntity(selectedEntity);
	SetSelectedEntity(nullptr);
}

bool Editor::CheckIsSelectedEntityValid()
{
	if(selectedEntity && !ecs::IsEntityHandleValid(selectedEntity))
		selectedEntity = nullptr;
	return selectedEntity;
}

void Editor::DrawEntityHeader()
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

void Editor::DrawEntityComps()
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
			editor::ComponentDrawMethods::Draw(compIter.GetCompHash(), compIter.GetComp<void>());
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

void Editor::DrawAddComp()
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
void Editor::DrawEntityActionsButton()
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

Vector2 Editor::SnapToGrid(const Vector2& worldPos) const {
	return {
		std::round((worldPos.x - m_gridOffset.x) / m_gridSize) * m_gridSize + m_gridOffset.x,
		std::round((worldPos.y - m_gridOffset.y) / m_gridSize) * m_gridSize + m_gridOffset.y
	};
}
void Editor::RenderGrid()
{
    if(!m_showGrid || (ST<Game>::Get()->GetState() == GAMESTATE::IN_GAME || ST<Game>::Get()->GetState() == GAMESTATE::PAUSE)) return;
    
    // Get necessary information from the renderer
    const auto& cameraData = ST<CameraController>::Get()->GetCameraData();
    Vector2 cameraPosition = cameraData.position;
    float zoom = cameraData.zoom;
    VkExtent2D viewport = ST<Engine>::Get()->_worldExtent;
    float halfWidth = viewport.width / (2.0f * zoom);
    float halfHeight = viewport.height / (2.0f * zoom);
    Vector2 topLeft = cameraPosition - Vector2(halfWidth, halfHeight);
    Vector2 bottomRight = cameraPosition + Vector2(halfWidth, halfHeight);
    topLeft -= m_gridOffset;
    bottomRight -= m_gridOffset;
    int startX = static_cast<int>(std::floor(topLeft.x / m_gridSize));
    int endX = static_cast<int>(std::ceil(bottomRight.x / m_gridSize));
    int startY = static_cast<int>(std::floor(topLeft.y / m_gridSize));
    int endY = static_cast<int>(std::ceil(bottomRight.y / m_gridSize));
    
    // Extract color components from Vector4
    Vector3 gridColorRGB(m_gridColor.x, m_gridColor.y, m_gridColor.z);
    float gridAlpha = m_gridColor.w;
    
    // Vertical lines
    for(int x = startX; x <= endX; ++x) {
        Vector2 start(x * m_gridSize + m_gridOffset.x, startY * m_gridSize + m_gridOffset.y);
        Vector2 end(x * m_gridSize + m_gridOffset.x, endY * m_gridSize + m_gridOffset.y);
        util::DrawLine(start, end, gridColorRGB, gridAlpha);
    }
    
    // Horizontal lines
    for(int y = startY; y <= endY; ++y) {
        Vector2 start(startX * m_gridSize + m_gridOffset.x, y * m_gridSize + m_gridOffset.y);
        Vector2 end(endX * m_gridSize + m_gridOffset.x, y * m_gridSize + m_gridOffset.y);
        util::DrawLine(start, end, gridColorRGB, gridAlpha);
    }
}
#endif