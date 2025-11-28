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

#include "UI/BarComponent.h"

void BarComponent::OnAttached()
{
	IUIComponent::OnAttached();
	SetPercentageFilled(1.0f);
}

void BarComponent::SetPercentageFilled(float percent)
{
	percent = std::clamp(percent, 0.0f, 1.0f);

	ecs::CompHandle<RectTransformComponent> rectTransform{ ecs::GetEntity(this)->GetComp<RectTransformComponent>() };
	rectTransform->SetLocalPosition(Vec2{ (percent - 1.0f) * 0.5f, 0.0f });
	rectTransform->SetLocalScale(Vec2{ percent, 1.0f });
}
