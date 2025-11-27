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

#include "UI/ButtonComponent.h"
#include "Engine/Input.h"
#include "Engine/Platform/Android/AndroidInputBridge.h"
#include "UI/SpriteComponent.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

namespace internal {

	bool TestClicked(RectTransformComponent& rectTransform, SpriteComponent& spriteComp, Vec2 mousePos)
	{
		return std::visit([&rectTransform, mousePos](auto& primitive) -> bool {
			return primitive.IsClicked(rectTransform, mousePos);
		}, spriteComp.GetPrimitive());
	}

}

void ButtonComponent::OnClicked()
{
	CONSOLE_LOG(LEVEL_INFO) << "CLICKED";
}

void ButtonComponent::OnAttached()
{
	ecs::GetEntity(this)->AddComp(SpriteComponent{});
	IUIComponent::OnAttached();
}

ButtonInputSystem::ButtonInputSystem()
	: System_Internal{ &ButtonInputSystem::CheckButtonInput }
	, pressed{}
	, released{}
{
}

bool ButtonInputSystem::PreRun()
{
#ifdef __ANDROID__
	pressed = AndroidInputBridge::State().justDown;
	released = AndroidInputBridge::State().justUp;
#else
	pressed = ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::M_LEFT);
	released = ST<KeyboardMouseInput>::Get()->GetIsReleased(KEY::M_LEFT);
#endif
	if (!(pressed || released))
		return false;
	pos = RetrieveMousePos();
	return true;
}

Vec2 ButtonInputSystem::RetrieveMousePos()
{
#ifdef __ANDROID__
	return Vec2{ AndroidInputBridge::State().x, AndroidInputBridge::State().y };
#else
	if (auto eventHandlerMousePos{ ST<EventsQueue>::Get()->RequestValueFromEventHandlers<Vec2>(Getters::MousePosViewport{}) })
		return eventHandlerMousePos.value();
	else
		return ST<KeyboardMouseInput>::Get()->GetMousePos();
#endif
}

void ButtonInputSystem::CheckButtonInput(ButtonComponent& buttonComp, SpriteComponent& spriteComp, RectTransformComponent& rectTransform)
{
	CONSOLE_LOG(LEVEL_INFO) << "Pos: " << pos.x << "," << pos.y;

	// Test if the button was clicked
	if (!TestClicked(rectTransform, spriteComp, pos))
		return;

	// TODO: Change sprite

	// If released, execute button function
	if (released)
		buttonComp.OnClicked();
}
