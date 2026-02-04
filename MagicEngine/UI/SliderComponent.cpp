/******************************************************************************/
/*!
\file   EventsQueue.cpp
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

#include "UI/SliderComponent.h"
#include "UI/ButtonComponent.h"
#include "Engine/EntityEvents.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Input.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "core/platform/platform.h"

SliderComponent::SliderComponent()
	: type{ UI_SLIDER_TYPE::KNOB }
	, isDragging{}
	, dragAmount{}
{
}

UI_SLIDER_TYPE SliderComponent::GetSliderType() const
{
	return type;
}

bool SliderComponent::IsDragging() const
{
	return isDragging;
}

RectTransformComponent* SliderComponent::GetSliderEntityTransform()
{
	return (sliderEntity ? sliderEntity->GetComp<RectTransformComponent>() : nullptr);
}

RectTransformComponent* SliderComponent::GetBackgroundEntityTransform()
{
	return (backgroundEntity ? backgroundEntity->GetComp<RectTransformComponent>() : nullptr);
}

float SliderComponent::GetDragAmount() const
{
	return dragAmount;
}

void SliderComponent::SetDragAmount(float newVal)
{
	dragAmount = newVal;
}

void SliderComponent::OnStart()
{
	bool success{};
	if (backgroundEntity)
		if (backgroundEntity->HasComp<ButtonComponent>() && backgroundEntity->HasComp<RectTransformComponent>())
		{
			auto entityEventsComp{ backgroundEntity->GetComp<EntityEventsComponent>() };
			entityEventsComp->Subscribe("OnButtonPressed", this, &SliderComponent::OnSliderPressed);
			entityEventsComp->Subscribe("OnButtonReleased", this, &SliderComponent::OnSliderReleased);
			success = true;
		}
	if (!sliderEntity || !sliderEntity->HasComp<RectTransformComponent>())
		success = false;

	if (!success)
		CONSOLE_LOG(LEVEL_ERROR) << "SliderComponent requires BackgroundEntity to have ButtonComponent and SliderEntity to have RectTransformComponent";
}

void SliderComponent::EditorDraw()
{
	if (gui::Combo typeCombo{ "Type", type == UI_SLIDER_TYPE::KNOB ? "Knob" : "Bar" })
	{
		if (gui::Selectable("Knob", type == UI_SLIDER_TYPE::KNOB))
			type = UI_SLIDER_TYPE::KNOB;
		if (gui::Selectable("Bar", type == UI_SLIDER_TYPE::BAR))
			type = UI_SLIDER_TYPE::BAR;
	}

	backgroundEntity.EditorDraw("Background");
	sliderEntity.EditorDraw("Slider Knob/Bar");
}

void SliderComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("type", type);
	ISerializeable::Serialize(writer);
}

void SliderComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("type", &type);
	ISerializeable::Deserialize(reader);
}

void SliderComponent::OnSliderPressed()
{
	isDragging = true;
}

void SliderComponent::OnSliderReleased()
{
	isDragging = false;
	auto entityEventsComp{ ecs::GetEntity(this)->GetComp<EntityEventsComponent>() };
	entityEventsComp->BroadcastAll("OnSliderValueFinalized", dragAmount);
	entityEventsComp->BroadcastAll("CallScriptFunc", std::string{ "OnSliderValueFinalized" });
}

SliderSystem::SliderSystem()
	: System_Internal{ &SliderSystem::UpdateSliderComp }
{
}

bool SliderSystem::PreRun()
{
	// Convert window position to UI space, accounting for letterboxing
	Vec2 rawPos = MagicInput::GetMousePos();
	float uiX, uiY;
	if (ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY)) {
		mousePos = Vec2{ uiX, uiY };
	}
	// If in letterbox area, keep last valid mousePos (slider continues dragging)
	return true;
}

void SliderSystem::UpdateSliderComp(SliderComponent& comp)
{
	if (!comp.IsDragging())
		return;

	RectTransformComponent* backgroundTransform{ comp.GetBackgroundEntityTransform() }, *sliderTransform{ comp.GetSliderEntityTransform() };
	Vec2 backgroundPos{ backgroundTransform->GetWorldPosition() }, backgroundScale{ backgroundTransform->GetWorldScale() };
	float xMin{ backgroundPos.x - backgroundScale.x * 0.5f }, xMax{ backgroundPos.x + backgroundScale.x * 0.5f };
	switch (comp.GetSliderType())
	{
	case UI_SLIDER_TYPE::KNOB: {
		float sliderHalfScaleX{ sliderTransform->GetWorldScale().x * 0.5f };
		xMin += sliderHalfScaleX;
		xMax -= sliderHalfScaleX;
		break;
	}
	case UI_SLIDER_TYPE::BAR:
		break;
	}
	float length{ xMax - xMin };
	float mouseXClamped{ std::clamp(mousePos.x, xMin, xMax) };
	comp.SetDragAmount((mouseXClamped - xMin) / length);

	switch (comp.GetSliderType())
	{
	case UI_SLIDER_TYPE::KNOB:
		sliderTransform->SetWorldPosition(Vec2{ mouseXClamped, sliderTransform->GetWorldPosition().y });
		break;
	case UI_SLIDER_TYPE::BAR:
		sliderTransform->SetWorldScale(Vec2{ comp.GetDragAmount() * backgroundScale.x, backgroundScale.y });
		sliderTransform->SetWorldPosition(Vec2{ xMin + length * comp.GetDragAmount() * 0.5f, sliderTransform->GetWorldPosition().y });
		break;
	}
}
