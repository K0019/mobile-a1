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
#include "CustomViewport.h"

#include "AnimatorComponent.h"
#include "SceneManagement.h"
#include "EditorHistory.h"


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
	switch (ST<Game>::Get()->GetState())
	{
	case GAMESTATE::IN_GAME:
	case GAMESTATE::PAUSE:
		isPlayMode = true;
	}
	bool isPauseMode{ ST<Game>::Get()->GetState() == GAMESTATE::PAUSE };
	

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
	static CameraData camera_data;
	if (ImGui::Button(isPlayMode ? ICON_FA_STOP : ICON_FA_PLAY, ImVec2(BUTTON_WIDTH, BUTTON_HEIGHT)) || Input::GetKeyPressed(KEY::F5))
	{
		if (!isPlayMode) // About to enter play mode
		{
			camera_data.position = ST<CameraController>::Get()->GetPosition();
			camera_data.zoom = ST<CameraController>::Get()->GetZoom();
			camera_data.targetZoom = ST<CameraController>::Get()->GetZoom();
		}
		else // About to exit play mode
		{
			ST<CameraController>::Get()->SetCameraData(camera_data);
		}
		ST<Game>::Get()->TogglePlayMode();
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
		ST<Game>::Get()->TogglePauseMode();
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

void CustomViewport::DrawImGuiWindow() {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
	if (disableMoving) {
		window_flags |= ImGuiWindowFlags_NoMove;
		disableMoving = false;
	}

	// Set size constraints for undocked windows
	if (!ImGui::GetCurrentWindow()->DockIsActive)
	{
		ImGui::SetNextWindowSizeConstraints(ImVec2(100, 100), ImVec2(FLT_MAX, FLT_MAX), MaintainAspectRatio, this);
	}

	// Begin main window
	if (ImGui::Begin(name.c_str(), nullptr, window_flags))
	{
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

		// Store viewport information
		viewportRenderSize = renderSize;
		windowPosAbsolute = windowPos;
		contentMin = ImVec2(padding.x, padding.y + titleBarHeight + playControlsHeight);
		contentMax = ImVec2(padding.x + renderSize.x,
			padding.y + renderSize.y + titleBarHeight + playControlsHeight);

		// Set position and render viewport image
		ImGui::SetCursorPos(ImVec2(padding.x, padding.y + titleBarHeight + playControlsHeight));
		ImGui::Image(
			ST<Engine>::Get()->_vulkan->getViewportDescriptorSet(),
			renderSize,
			ImVec2(0, 0), ImVec2(1, 1)
		);

		if (ImGui::BeginDragDropTarget())
		{
			if (ImGuiPayload const* payload = ImGui::AcceptDragDropPayload("SPRITE_ID"))
			{
				if (ImGui::IsMouseReleased(0))
				{
					ecs::EntityHandle entity{ ecs::CreateEntity() };
					ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ entity });
					size_t ID = *static_cast<size_t*>(payload->Data);
					const auto& sprite = ResourceManager::GetSprite(ID);
					entity->GetTransform().SetLocal(0.5f, Input::GetMousePosWorld(), { static_cast<float>(sprite.width), static_cast<float>(sprite.height) }, 0.0f);
					entity->AddCompNow(RenderComponent{ ID });
					ST<Editor>::Get()->SetSelectedEntity(entity);
				}
			}
			else if (ImGuiPayload const* payload_entity = ImGui::AcceptDragDropPayload("PREFAB"))
			{
				if (ImGui::IsMouseReleased(0))
				{
					std::string prefabName{ static_cast<char*>(payload_entity->Data) };
					ecs::EntityHandle entity = PrefabManager::LoadPrefab(prefabName);
					ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ entity });
					entity->GetTransform().SetWorldPosition(Input::GetMousePosWorld());
					ST<Editor>::Get()->SetSelectedEntity(entity);
				}
			}
			else if (ImGuiPayload const* payload_anim = ImGui::AcceptDragDropPayload("ANIM_HASH"))
			{
				if (ImGui::IsMouseReleased(0))
				{
					size_t animHash = *static_cast<size_t*>(payload_anim->Data);
					ecs::EntityHandle entity = ecs::CreateEntity();
					ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ entity });
					const auto& anim = ResourceManager::GetAnimation(animHash);
					entity->AddCompNow(RenderComponent{});
					entity->AddCompNow(AnimatorComponent{ animHash });
					entity->GetTransform().SetLocal(0.5f, Input::GetMousePosWorld(), { static_cast<float>(anim.Width), static_cast<float>(anim.Height) }, 0.0f);
					ST<Editor>::Get()->SetSelectedEntity(entity);
				}
			}
			ImGui::EndDragDropTarget();
		}

		// Create gizmo overlay
		ImGui::SetItemAllowOverlap();
		ImGui::SetCursorPos(ImVec2(padding.x, padding.y + titleBarHeight + playControlsHeight));

		// Push a unique ID for the child window to avoid conflicts
		ImGui::PushID("GizmoOverlay");
		bool childBegin = ImGui::BeginChild("GizmoContent", renderSize, false,
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoBackground);
		if (childBegin)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			ST<Editor>::Get()->DrawGizmoInViewport(drawList);
			ImGui::EndChild();
		}
		ImGui::PopID();
	}
	ImGui::End();
	ImGui::PopStyleVar();
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

Transform CustomViewport::WorldToWindowTransform(const Transform& worldTransform) const {
	Transform viewTransform;
#ifdef IMGUI_ENABLED
	auto WORLD = ST<Engine>::Get()->_viewportExtent;
#else
	auto WORLD = ST<Engine>::Get()->_worldExtent;
	auto WINDOW = ST<Engine>::Get()->_windowExtent;
#endif
	Vector2 cameraPos = ST<CameraController>::Get()->GetPosition();
	float zoom = ST<CameraController>::Get()->GetZoom();
	Vector2 worldPos = worldTransform.GetWorldPosition();
	Vector2 cameraRelativePos = (worldPos - cameraPos) * zoom;

#ifdef IMGUI_ENABLED
	// Original viewport-based transformation
	double viewportX = (cameraRelativePos.x + WORLD.width / 2.0f) / WORLD.width;
	double viewportY = (cameraRelativePos.y + WORLD.height / 2.0f) / WORLD.height;
	Vector2 viewPos = {
		static_cast<float>(windowPosAbsolute.x + contentMin.x + viewportX * viewportRenderSize.x),
		static_cast<float>(windowPosAbsolute.y + contentMin.y + (1.0 - viewportY) * viewportRenderSize.y)
	};
#else
	// Full screen rendering with correct scaling
	float scaleX = static_cast<float>(WINDOW.width) / WORLD.width;
	float scaleY = static_cast<float>(WINDOW.height) / WORLD.height;
	Vector2 viewPos = {
		static_cast<float>((cameraRelativePos.x + WORLD.width / 2.0f) * scaleX),
		static_cast<float>((WORLD.height / 2.0f - cameraRelativePos.y) * scaleY)
	};
#endif

	viewTransform.SetLocalPosition(viewPos);
	Vector2 worldScale = worldTransform.GetWorldScale();

#ifdef IMGUI_ENABLED
	// Original viewport scaling
	Vector2 viewScale = {
		worldScale.x * (viewportRenderSize.x / WORLD.width) * zoom,
		worldScale.y * (viewportRenderSize.y / WORLD.height) * zoom
	};
#else
	// Full screen scaling with window-to-world ratio
	Vector2 viewScale = {
		worldScale.x * zoom * (static_cast<float>(WINDOW.width) / WORLD.width),
		worldScale.y * zoom * (static_cast<float>(WINDOW.height) / WORLD.height)
	};
#endif

	viewTransform.SetLocalRotation(worldTransform.GetWorldRotation());
	viewTransform.SetLocalScale(viewScale);
	return viewTransform;
}

Vector2 CustomViewport::WindowToWorldPosition(const Vector2& inWindowPos) const {

#ifdef IMGUI_ENABLED
	auto WORLD = ST<Engine>::Get()->_viewportExtent;
#else
	auto WORLD = ST<Engine>::Get()->_worldExtent;
	auto WINDOW = ST<Engine>::Get()->_windowExtent;
#endif
	Vector2 cameraPos = ST<CameraController>::Get()->GetPosition();
	float zoom = ST<CameraController>::Get()->GetZoom();

#ifdef IMGUI_ENABLED
	// Original viewport-based transformation
	double viewportX = (inWindowPos.x - (windowPosAbsolute.x + contentMin.x)) / viewportRenderSize.x;
	double viewportY = 1.0 - (inWindowPos.y - (windowPosAbsolute.y + contentMin.y)) / viewportRenderSize.y;
	Vector2 worldPos = {
		static_cast<float>((viewportX - 0.5) * WORLD.width / zoom + cameraPos.x),
		static_cast<float>((viewportY - 0.5) * WORLD.height / zoom + cameraPos.y)
	};
#else
	// Full screen transformation
	float scaleX = WORLD.width / static_cast<float>(WINDOW.width);
	float scaleY = WORLD.height / static_cast<float>(WINDOW.height);

	Vector2 worldPos = {
		static_cast<float>((inWindowPos.x * scaleX - WORLD.width / 2.0f) / zoom + cameraPos.x),
		static_cast<float>((WORLD.height / 2.0f - inWindowPos.y * scaleY) / zoom + cameraPos.y)
	};
#endif

	return worldPos;
}


bool CustomViewport::IsMouseInViewport(const Vector2& mousePos) const
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

#else
	UNREFERENCED_PARAMETER(mousePos);
#endif

	// Check for any popups or modal windows that might be blocking
	/*if(g.NavWindow && g.NavWindow->RootWindow != window->RootWindow) {
		return false;
	}*/

	return true;
}

void CustomViewport::SetDisableMoving(bool disable) {
	disableMoving = disable;
}
Vector2 CustomViewport::GetViewportRenderSize() const
{
	return { viewportRenderSize.x, viewportRenderSize.y };
}

