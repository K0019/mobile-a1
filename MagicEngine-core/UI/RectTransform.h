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
#include "Editor/IHiddenComponent.h"

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

	void EditorDraw();

	void OnAttached() override;

};
