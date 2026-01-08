/******************************************************************************/
/*!
\file   CustomViewport.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the CustomViewport class, which represents a custom viewport for rendering graphics.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Graphics/CustomViewport.h"
#include "Engine/Input.h"
#include "Engine/PrefabManager.h"
#include "Game/GameSystems.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"

#include "Engine/SceneManagement.h"
#include "Editor/EditorHistory.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

CustomViewport::CustomViewport(unsigned int width, unsigned int height)
	: WindowBase{ "Viewport", { 1366, 768 }, gui::FLAG_WINDOW::NO_SCROLL_BAR | gui::FLAG_WINDOW::NO_SCROLL_WITH_MOUSE }
	, width{ width }
	, height{ height }
	, aspect_ratio{ static_cast<float>(width) / static_cast<float>(height) }
	, titleBarHeight{}
	, name{ ICON_FA_GAMEPAD " Scene" }
	, camera{ vec3(0.0f, 1.0f, -1.5f), vec3(0.0f, 0.5f, 0.0f), vec3(0.0f, 1.0f, 0.0f) }
	, eventHandle_viewportMousePos{}
{
}

void CustomViewport::OnAttached()
{
	// Using this pointer as we guarantee that there will only be 1 custom viewport ever...
	eventHandle_viewportMousePos = ST<EventsQueue>::Get()->AddEventHandlerFunc<Getters::MousePosViewport>([this](const Getters::MousePosViewport&) -> Vec2 {
		Vec2 mousePos{ ST<KeyboardMouseInput>::Get()->GetMousePos() };
		Vec2 windowExtent{ static_cast<float>(Core::Display().GetWidth()), static_cast<float>(Core::Display().GetHeight()) };

		return Vec2{
			(mousePos.x - (windowPosAbsolute.x + contentMin.x)) / viewportRenderSize.x * windowExtent.x,
			(mousePos.y - (windowPosAbsolute.y + contentMin.y)) / viewportRenderSize.y * windowExtent.y
		};
	});
}

void CustomViewport::OnDetached()
{
	ST<EventsQueue>::Get()->DeleteEventHandler(eventHandle_viewportMousePos);
}

void CustomViewport::DrawContainer(int id)
{
#ifdef IMGUI_ENABLED
	gui::SetStyleVar windowPadding{ gui::FLAG_STYLE_VAR::WINDOW_PADDING, gui::Vec2{ 0, 0 } };

	// Set size constraints for undocked windows
	if (!ImGui::GetCurrentWindow()->DockIsActive)
		ImGui::SetNextWindowSizeConstraints(gui::Vec2{ 100, 100 }, gui::Vec2{ FLT_MAX, FLT_MAX }, MaintainAspectRatio, this);
#endif

	WindowBase::DrawContainer(id);
}

void CustomViewport::DrawWindow()
{
	// TODO: Have some update for editor windows so they don't update in the DrawWindow() function

	// Handle events
	EventsReader<Events::ResizeViewport> resizeViewportEvent{};
	while (auto event{ resizeViewportEvent.ExtractEvent() })
		Resize(event->width, event->height);

	// Camera movement (should be moved to an input update section)
	UpdateCameraControl();
	// Camera upload (should also be moved...)
	ST<GraphicsMain>::Get()->SetViewCamera(GetViewportCamera());


#ifdef IMGUI_ENABLED
	const float playControlsHeight = 22.0f; // Height of play controls bar
	DrawPlayControls();

	ImVec2 windowPos = ImGui::GetWindowPos();
	ImVec2 contentRegionAvail = ImGui::GetContentRegionAvail();
	titleBarHeight = ImGui::GetFrameHeight();
	bool isDocked = ImGui::GetCurrentWindow()->DockIsActive;

	// Subtract play controls height from available space
	contentRegionAvail.y -= playControlsHeight;
	ImVec2 renderSize;
	ImVec2 padding;

	if (isDocked)
	{
		// For docked windows, fit content to available area while maintaining aspect ratio
		float contentAspectRatio = contentRegionAvail.x / contentRegionAvail.y;
		if (contentAspectRatio > aspect_ratio) {
			renderSize.y = contentRegionAvail.y;
			renderSize.x = renderSize.y * aspect_ratio;
		}
		else {
			renderSize.x = contentRegionAvail.x;
			renderSize.y = renderSize.x / aspect_ratio;
		}
		padding = ImVec2((contentRegionAvail.x - renderSize.x) * 0.5f,
			(contentRegionAvail.y - renderSize.y) * 0.5f);
	}
	else
	{
		renderSize = contentRegionAvail;
		padding = ImVec2(0, 0);
	}


	//=============================
	// Store viewport information
	viewportRenderSize = renderSize;
	windowPosAbsolute = windowPos;
	contentMin = ImVec2(padding.x, padding.y + titleBarHeight + playControlsHeight);
	contentMax = ImVec2(padding.x + renderSize.x,
		padding.y + renderSize.y + titleBarHeight + playControlsHeight);

	// 1) Draw the scene texture first
	ImGui::SetCursorPos(ImVec2(padding.x, padding.y + titleBarHeight + playControlsHeight));
	if (auto sceneColorID = ST<GraphicsMain>::Get()->GetImGuiContext()
		.GetTransientRegistry().QueryBindlessID("ImGuiSceneView"))
	{
		ImGui::Image(*sceneColorID, renderSize, ImVec2(0, 0), ImVec2(1, 1));
	}

	if (ST<GameSystemsManager>::Get()->GetState() != GAMESTATE::IN_GAME && ST<GameSystemsManager>::Get()->GetState() != GAMESTATE::PAUSE)
	{
		// Left click to pick objects (when not dragging camera with right mouse)
	static bool wasLeftMouseDown = false;
	bool isLeftMouseDown = Input::GetMouseButtonUp(MouseButton::Left);
	bool leftClickJustPressed = isLeftMouseDown && !wasLeftMouseDown;
	wasLeftMouseDown = isLeftMouseDown;

	if (leftClickJustPressed && !Input::GetMouseButton(MouseButton::Right))
	{
		ImVec2 mousePos = ImGui::GetMousePos();

		// Convert to viewport-relative coordinates first
		float viewportRelativeX = mousePos.x - (windowPosAbsolute.x + contentMin.x);
		float viewportRelativeY = mousePos.y - (windowPosAbsolute.y + contentMin.y);

		// Check if mouse is within the viewport bounds
		bool isInViewport = (viewportRelativeX >= 0 && viewportRelativeX < viewportRenderSize.x &&
			viewportRelativeY >= 0 && viewportRelativeY < viewportRenderSize.y);

			if (isInViewport)
			{

				// Get the actual render target dimensions
				auto sceneColorID = ST<GraphicsMain>::Get()->GetImGuiContext()
					.GetTransientRegistry().QueryBindlessID("ImGuiSceneView");

				// Calculate the actual render target size
				// You may need to get this from your graphics system
				// For now, assuming it matches your configured width/height
				float renderTargetWidth = static_cast<float>(width);
				float renderTargetHeight = static_cast<float>(height);

				// Convert viewport coordinates to render target coordinates
				float normalizedX = viewportRelativeX / viewportRenderSize.x;
				float normalizedY = viewportRelativeY / viewportRenderSize.y;

				int renderX = static_cast<int>(normalizedX * renderTargetWidth);
				int renderY = static_cast<int>(normalizedY * renderTargetHeight);

				// Clamp to render target bounds
				renderX = std::max(0, std::min(renderX, static_cast<int>(renderTargetWidth) - 1));
				renderY = std::max(0, std::min(renderY, static_cast<int>(renderTargetHeight) - 1));

				if (ST<GraphicsMain>::Get()->RequestObjPick(renderX, renderY))
				{
					// Debug output
					// std::cout << "Object pick requested at render target position (" << screenX << ", " << screenY << ")\n";
					// std::cout << "  Viewport position: (" << viewportRelativeX << ", " << viewportRelativeY << ")\n";
					// std::cout << "  Render target size: " << renderTargetWidth << "x" << renderTargetHeight << "\n";
				}
			}
		}

	// Check for pick rsult from previous frame
	ecs::EntityHandle pickedEntity = ST<GraphicsMain>::Get()->PreviousPick();
	if (pickedEntity)
	{
		ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorSelectEntity{ pickedEntity });
	}


	m_gizmo.Draw(ST<EventsQueue>::Get()->RequestValueFromEventHandlers<ecs::EntityHandle>(Getters::EditorSelectedEntity{}).value_or(nullptr));

	//==========================

	gui::PayloadTarget<std::string>("PREFAB", [camera = &camera](const std::string& prefabName) -> void {
		ecs::EntityHandle entity{ PrefabManager::LoadPrefab(prefabName) };
		ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ entity });
		entity->GetTransform().SetWorldPosition(camera->getPosition());
		ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorSelectEntity{ entity });
	});
	}
#endif
}

void CustomViewport::Init(unsigned newWidth, unsigned newHeight)
{
	width = newWidth;
	height = newHeight;
	aspect_ratio = static_cast<float>(newWidth) / static_cast<float>(newHeight);

	CONSOLE_LOG(LEVEL_INFO) << "Custom viewport with dimensions " << newWidth << " * " << newHeight << " Initialised.";
}

void CustomViewport::Resize(unsigned newWidth, unsigned newHeight)
{
	if (newWidth == this->width && newHeight == this->height) {
		return;
	}

	this->width = newWidth;
	this->height = newHeight;
	this->aspect_ratio = static_cast<float>(newWidth) / static_cast<float>(newHeight);

	CONSOLE_LOG(LEVEL_INFO) << "Custom viewport resized to dimensions " << width << " * " << height << " with aspect ratio " << aspect_ratio;
}

void CustomViewport::UpdateCameraControl()
{
	static Vec2 lastMousePos;
	vec2 mouse_delta = ST<KeyboardMouseInput>::Get()->GetMousePos() - lastMousePos;
	lastMousePos = ST<KeyboardMouseInput>::Get()->GetMousePos();

	if (ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::M_RIGHT))
	{
		camera.movement_.forward_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::W);
		camera.movement_.backward_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::S);
		camera.movement_.left_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::A);
		camera.movement_.right_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::D);
		camera.movement_.up_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::NUM_1);
		camera.movement_.down_ = ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::NUM_2);
	}
	else
		// Reset keys
		camera.movement_ = CameraPositioner_FirstPerson::Movement{};


	camera.update(GameTime::Dt(), mouse_delta, ST<KeyboardMouseInput>::Get()->GetIsDown(KEY::M_RIGHT));
}

#ifdef IMGUI_ENABLED

void CustomViewport::DrawPlayControls() {
	constexpr float TOOLBAR_HEIGHT = 22.0f;
	constexpr float BUTTON_WIDTH = 30.0f;
	constexpr float BUTTON_HEIGHT = 20.0f;

	// Save current style
	ImGuiStyle& style = ImGui::GetStyle();
	float originalSpacing = style.ItemSpacing.x;
	style.ItemSpacing.x = 0;

	// Create toolbar with Unity-like dark theme
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.165f, 0.165f, 0.165f, 1.0f)); // Unity's darker toolbar
	ImGui::BeginChild("PlayControlsBar", ImVec2(ImGui::GetContentRegionAvail().x, TOOLBAR_HEIGHT), false);

	// Center the controls
	float windowWidth = ImGui::GetWindowWidth();
	float controlsWidth = BUTTON_WIDTH * 3; // Width for play, pause, step buttons
	float startX = (windowWidth - controlsWidth) * 0.5f;
	ImGui::SetCursorPosX(startX);

	// Button styles
	const ImVec4 activeColor(0.165f, 0.47f, 0.165f, 1.0f);      // Unity green when playing
	const ImVec4 inactiveColor(0.165f, 0.165f, 0.165f, 1.0f);   // Dark when not playing
	const ImVec4 hoverColor(0.25f, 0.25f, 0.25f, 1.0f);         // Lighter on hover
	const ImVec4 activeHoverColor(0.2f, 0.55f, 0.2f, 1.0f);     // Lighter green on hover

	// Common button style
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 2));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);      // Square buttons like Unity
	ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);    // Border for buttons

	// Play/Stop button
	bool isPlayMode{ false };
	switch (ST<GameSystemsManager>::Get()->GetState())
	{
	case GAMESTATE::IN_GAME:
	case GAMESTATE::PAUSE:
		isPlayMode = true;
	}
	bool isPauseMode{ ST<GameSystemsManager>::Get()->GetState() == GAMESTATE::PAUSE };
	

	ImGui::PushStyleColor(ImGuiCol_Button, isPlayMode ? activeColor : inactiveColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, isPlayMode ? activeHoverColor : hoverColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, isPlayMode ? activeHoverColor : hoverColor);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));

	// Add a subtle glow effect when in play mode
	if (isPlayMode) {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 1.0f, 0.6f, 1.0f));
	}
	else {
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));
	}
	//static CameraData camera_data;
	if (ImGui::Button(isPlayMode ? ICON_FA_STOP : ICON_FA_PLAY, ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)) || ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::F5))
	{
		/*if (!isPlayMode) // About to enter play mode
		{
			camera_data.position = ST<CameraController>::Get()->GetPosition();
			camera_data.zoom = ST<CameraController>::Get()->GetZoom();
			camera_data.targetZoom = ST<CameraController>::Get()->GetZoom();
		}
		else // About to exit play mode
		{
			ST<CameraController>::Get()->SetCameraData(camera_data);
		}*/
		Messaging::BroadcastAll("EngineTogglePlayMode");
	}
	ImGui::PopStyleColor(5); // Text + Button colors + Border

	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip(isPlayMode ? "Stop (F5)" : "Play (F5)");
	}

	// Pause button (slightly darker when active)
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, inactiveColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text,
		(isPauseMode ? ImVec4(0.6f, 0.6f, 0.6f, 1.0f) : ImVec4(0.8f, 0.8f, 0.8f, 1.0f)));

	if (ImGui::Button(ICON_FA_PAUSE, ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)) && isPlayMode)
	{
		Messaging::BroadcastAll("EngineTogglePauseMode");
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Pause");
	}
	ImGui::PopStyleColor(5);

	// Step button
	ImGui::SameLine();
	ImGui::PushStyleColor(ImGuiCol_Button, inactiveColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoverColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, hoverColor);
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.8f, 1.0f));

	if (ImGui::Button(ICON_FA_FORWARD_STEP, ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT))) {
		// TODO: Implement step functionality
	}
	if (ImGui::IsItemHovered()) {
		ImGui::SetTooltip("Step");
	}
	ImGui::PopStyleColor(5);

	ImGui::PopStyleVar(3); // Frame padding, rounding, border
	ImGui::EndChild();
	ImGui::PopStyleColor(); // Toolbar background

	// Restore original style
	style.ItemSpacing.x = originalSpacing;
}

void CustomViewport::MaintainAspectRatio(ImGuiSizeCallbackData* data) {
	CustomViewport* viewport = static_cast<CustomViewport*>(data->UserData);
	float aspect_ratio = viewport->aspect_ratio;
	float titleBarHeight = ImGui::GetFrameHeight();
	float playControlsHeight = 22.0f; // Match the height used in DrawPlayControls

	// Account for both title bar and play controls in available height
	float availableHeight = data->DesiredSize.y - (titleBarHeight + playControlsHeight);
	float desiredWidth = availableHeight * aspect_ratio;

	if (desiredWidth > data->DesiredSize.x)
	{
		desiredWidth = data->DesiredSize.x;
		availableHeight = desiredWidth / aspect_ratio;
	}

	data->DesiredSize.x = desiredWidth;
	data->DesiredSize.y = availableHeight + titleBarHeight + playControlsHeight;
}

#endif

Transform CustomViewport::WorldToWindowTransform([[maybe_unused]] const Transform& worldTransform) const {
	Transform viewTransform;
#ifdef IMGUI_ENABLED
	auto WORLD = ST<GraphicsWindow>::Get()->GetViewportExtent();
#else
	auto WORLD = ST<GraphicsWindow>::Get()->GetWorldExtent();
	auto WINDOW = ST<GraphicsWindow>::Get()->GetWindowExtent();
#endif

	CONSOLE_LOG_UNIMPLEMENTED() << "Viewport, world to window position conversion";

//	Vec3 cameraPos = ST<CameraController>::Get()->GetPosition();
//	float zoom = ST<CameraController>::Get()->GetZoom();
//	Vec3 worldPos = worldTransform.GetWorldPosition();
//	Vec3 cameraRelativePos = (worldPos - cameraPos) * zoom;
//
//#ifdef IMGUI_ENABLED
//	// Original viewport-based transformation
//	double viewportX = (cameraRelativePos.x + WORLD.width / 2.0f) / WORLD.width;
//	double viewportY = (cameraRelativePos.y + WORLD.height / 2.0f) / WORLD.height;
//	Vec2 viewPos = {
//		static_cast<float>(windowPosAbsolute.x + contentMin.x + viewportX * viewportRenderSize.x),
//		static_cast<float>(windowPosAbsolute.y + contentMin.y + (1.0 - viewportY) * viewportRenderSize.y)
//	};
//#else
//	// Full screen rendering with correct scaling
//	float scaleX = static_cast<float>(WINDOW.width) / WORLD.width;
//	float scaleY = static_cast<float>(WINDOW.height) / WORLD.height;
//	Vec2 viewPos = {
//		static_cast<float>((cameraRelativePos.x + WORLD.width / 2.0f) * scaleX),
//		static_cast<float>((WORLD.height / 2.0f - cameraRelativePos.y) * scaleY)
//	};
//#endif
//
//	viewTransform.SetLocalPosition(viewPos);
//	Vec2 worldScale = worldTransform.GetWorldScale();
//
//#ifdef IMGUI_ENABLED
//	// Original viewport scaling
//	Vec2 viewScale = {
//		worldScale.x * (viewportRenderSize.x / WORLD.width) * zoom,
//		worldScale.y * (viewportRenderSize.y / WORLD.height) * zoom
//	};
//#else
//	// Full screen scaling with window-to-world ratio
//	Vec2 viewScale = {
//		worldScale.x * zoom * (static_cast<float>(WINDOW.width) / WORLD.width),
//		worldScale.y * zoom * (static_cast<float>(WINDOW.height) / WORLD.height)
//	};
//#endif
//
//	viewTransform.SetLocalRotation(worldTransform.GetWorldRotation());
//	viewTransform.SetLocalScale(viewScale);
	return viewTransform;
}

Vec3 CustomViewport::WindowToWorldPosition([[maybe_unused]] const Vec2& inWindowPos) const {

/*
#ifdef IMGUI_ENABLED
	auto WORLD = ST<Engine>::Get()->_viewportExtent;
#else
	auto WORLD = ST<Engine>::Get()->_worldExtent;
	auto WINDOW = ST<Engine>::Get()->_windowExtent;
#endif
	Vec3 cameraPos = ST<CameraController>::Get()->GetPosition();
	float zoom = ST<CameraController>::Get()->GetZoom();

#ifdef IMGUI_ENABLED
	// Original viewport-based transformation
	double viewportX = (inWindowPos.x - (windowPosAbsolute.x + contentMin.x)) / viewportRenderSize.x;
	double viewportY = 1.0 - (inWindowPos.y - (windowPosAbsolute.y + contentMin.y)) / viewportRenderSize.y;
	// TODO 3D: MOST CERTAINLY WRONG! This math needs fixing.
	Vec3 worldPos = {
		static_cast<float>((viewportX - 0.5) * WORLD.width / zoom + cameraPos.x),
		static_cast<float>((viewportY - 0.5) * WORLD.height / zoom + cameraPos.y),
		cameraPos.z
	};
#else
	// Full screen transformation
	float scaleX = WORLD.width / static_cast<float>(WINDOW.width);
	float scaleY = WORLD.height / static_cast<float>(WINDOW.height);

	Vec3 worldPos = {
		static_cast<float>((inWindowPos.x * scaleX - WORLD.width / 2.0f) / zoom + cameraPos.x),
		static_cast<float>((WORLD.height / 2.0f - inWindowPos.y * scaleY) / zoom + cameraPos.y)
	};
#endif
*/

	return {};
}


bool CustomViewport::IsMouseInViewport([[maybe_unused]] const Vec2& mousePos) const
{
#ifdef IMGUI_ENABLED
	bool within_viewport = mousePos.x >= windowPosAbsolute.x + contentMin.x &&
		mousePos.x < windowPosAbsolute.x + contentMin.x + viewportRenderSize.x &&
		mousePos.y >= windowPosAbsolute.y + contentMin.y &&
		mousePos.y < windowPosAbsolute.y + contentMin.y + viewportRenderSize.y;

	if (!within_viewport) {
		return false;
	}

	ImGuiContext& g = *GImGui;
	ImGuiWindow* window = ImGui::FindWindowByName(name.c_str());
	if (!window || window->Hidden) {
		return false;
	}

	// Check if we're the topmost window at the mouse position
	if (g.HoveredWindow) {
		ImGuiWindow* root_window = g.HoveredWindow->RootWindow;
		if (root_window != window->RootWindow) {
			return false;
		}
	}
#endif

	// Check for any popups or modal windows that might be blocking
	/*if(g.NavWindow && g.NavWindow->RootWindow != window->RootWindow) {
		return false;
	}*/

	return true;
}

Vec2 CustomViewport::GetViewportRenderSize() const
{
	return { viewportRenderSize.x, viewportRenderSize.y };
}

Camera CustomViewport::GetViewportCamera() const
{
	return Camera{ camera };
}

namespace editor {

	bool SelectedEntityBorderDrawSystem::PreRun()
	{
		// TODO 3D: Editor, draw entity bounding box
		//if (ecs::EntityHandle selectedEntity{ ST<EventsQueue>::Get()->RequestValueFromEventHandlers<ecs::EntityHandle>(Getters::EditorSelectedEntity{}).value_or(nullptr) })
		//{
		//	Transform textTransform;
		//	const Transform* transform{};
		//	if (ecs::CompHandle<TextComponent> textComp{ selectedEntity->GetComp<TextComponent>() })
		//	{
		//		textTransform = selectedEntity->GetComp<TextComponent>()->GetWorldTextTransform();
		//		transform = &textTransform;
		//	}
		//	else
		//	{
		//		transform = &selectedEntity->GetTransform();
		//	}
		//	//util::DrawBoundingBox(*transform, { 0.0f, 1.0f, 0.0f });
		//}
		return false;
	}

}
