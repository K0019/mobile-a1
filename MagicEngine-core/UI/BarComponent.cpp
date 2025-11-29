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
#include "UI/SpriteComponent.h"
#include "Editor/Containers/GUICollection.h"

BarComponent::BarComponent()
	: modifyUV{ false }
{
}

void BarComponent::OnAttached()
{
	IUIComponent::OnAttached();
	SetPercentageFilled(1.0f);
}

void BarComponent::EditorDraw()
{
	gui::Checkbox("Modify UV", &modifyUV);
}

void BarComponent::SetPercentageFilled(float percent)
{
	percent = std::clamp(percent, 0.0f, 1.0f);

	ecs::CompHandle<RectTransformComponent> rectTransform{ ecs::GetEntity(this)->GetComp<RectTransformComponent>() };
	rectTransform->SetLocalPosition(Vec2{ (percent - 1.0f) * 0.5f, 0.0f });
	rectTransform->SetLocalScale(Vec2{ percent, 1.0f });

	if (modifyUV)
		if (ecs::CompHandle<SpriteComponent> spriteComp{ ecs::GetEntity(this)->GetComp<SpriteComponent>() })
			spriteComp->VisitPrimitive([percent]([[maybe_unused]] auto& primitive) -> void {
				using T = std::decay_t<decltype(primitive)>;
				if constexpr (std::is_same_v<T, Primitive2DImage>)
					primitive.SetUV(Vec2{}, Vec2{ percent, 1.0f });
			});
}
