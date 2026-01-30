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
#include "ECS/IEditorComponent.h"
#include "UI/IUIComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "ECS/EntityUID.h"

enum class UI_SLIDER_TYPE
{
	KNOB,
	BAR
};

class SliderComponent
	: public IRegisteredComponent<SliderComponent>
	, public IEditorComponent<SliderComponent>
	, public IGameComponentCallbacks<SliderComponent>
{
public:
	SliderComponent();

	UI_SLIDER_TYPE GetSliderType() const;
	bool IsDragging() const;
	RectTransformComponent* GetSliderEntityTransform();
	RectTransformComponent* GetBackgroundEntityTransform();
	float GetDragAmount() const;

	void SetDragAmount(float newVal);

	void OnStart() override;

	void EditorDraw() override;
	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

private:
	void OnSliderPressed();
	void OnSliderReleased();

private:
	UI_SLIDER_TYPE type;
	EntityReference sliderEntity; // Visual effect of slider
	EntityReference backgroundEntity; // Has button
	bool isDragging;
	float dragAmount;

public:
	property_vtable()
};
property_begin(SliderComponent)
{
	property_var(sliderEntity),
	property_var(backgroundEntity),
	property_var(dragAmount)
}
property_vend_h(SliderComponent)

class SliderSystem : public ecs::System<SliderSystem, SliderComponent>
{
public:
	SliderSystem();

	bool PreRun() override;

private:
	void UpdateSliderComp(SliderComponent& comp);

private:
	Vec2 mousePos;

};
