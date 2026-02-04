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
#include "ECS/IRegisteredComponent.h"
#include "ECS/IHiddenComponent.h"

// Horizontal anchor for UI elements at different aspect ratios
// At 16:9 (reference), all anchors produce the same position
// At wider ratios, Left stays left, Right stays right, Center shifts with extra space
enum class UIAnchorX : uint8_t
{
	Left,    // Distance from left edge is preserved
	Center,  // Element shifts with center (default - maintains current behavior)
	Right    // Distance from right edge is preserved
};

class RectTransformComponent
	: public IRegisteredComponent<RectTransformComponent>
	, public IHiddenComponent<RectTransformComponent>
	, public ecs::IComponentCallbacks
{
public:
	uint16_t GetLayer() const;
	void SetLayer(uint16_t layer);
	Vec2 GetLocalPosition() const;
	void SetLocalPosition(Vec2 newPos);
	Vec2 GetWorldPosition() const;
	void SetWorldPosition(Vec2 newPos);
	float GetRotation() const;
	void SetRotation(float newRot);
	Vec2 GetLocalScale() const;
	Vec2 GetWorldScale() const;
	void SetLocalScale(Vec2 newScale);
	void SetWorldScale(Vec2 newScale);

	// Horizontal anchor - determines how element shifts at different aspect ratios
	UIAnchorX GetAnchorX() const { return anchorX; }
	void SetAnchorX(UIAnchorX anchor) { anchorX = anchor; }

	void EditorDraw();

	void OnAttached() override;

private:
	UIAnchorX anchorX = UIAnchorX::Center;  // Default: center (maintains current behavior at 16:9)
};
