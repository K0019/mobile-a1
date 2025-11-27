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
#include "Engine/EntityEvents.h"
#include "Editor/Containers/GUICollection.h"

namespace internal {

	bool TestClicked(RectTransformComponent& rectTransform, SpriteComponent& spriteComp, Vec2 mousePos)
	{
		return spriteComp.template VisitPrimitive<bool>([&rectTransform, mousePos](auto& primitive) -> bool {
			return primitive.IsClicked(rectTransform, mousePos);
		});
	}

}

ButtonComponent::ButtonComponent()
	: isPressed{}
{
}

void ButtonComponent::OnPressed()
{
	isPressed = true;
	SwapPrimitive();
}

bool ButtonComponent::GetIsPressed() const
{
	return isPressed;
}

void ButtonComponent::ResetPressState()
{
	if (!isPressed)
		return;

	isPressed = false;
	SwapPrimitive();
}

void ButtonComponent::OnClicked()
{
	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("CallScriptFunc", std::string{ "OnButtonClicked" });
}

void ButtonComponent::OnAttached()
{
	ecs::GetEntity(this)->AddComp(SpriteComponent{});
	IUIComponent::OnAttached();
}

void ButtonComponent::EditorDraw()
{
	if (!ecs::GetEntity(this)->HasComp<SpriteComponent>())
	{
		gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 1.0f, 0.2f, 0.2f, 1.0f } };
		gui::TextWrapped("SpriteComponent IS REQUIRED for button to work. Please attach a SpriteComponent.");
		gui::Separator();
	}

	bool hasAlternatePrimitive{ otherPrimitive.has_value() };
	if (gui::Checkbox("Change Sprite When Pressed", &hasAlternatePrimitive))
	{
		if (hasAlternatePrimitive)
			otherPrimitive = Primitive2DHolder{};
		else
			otherPrimitive.reset();
	}

	if (otherPrimitive)
		otherPrimitive->EditorDraw();
}

void ButtonComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("hasPrimitive", otherPrimitive.has_value());
	if (otherPrimitive)
		otherPrimitive->Serialize(writer);
}

void ButtonComponent::Deserialize(Deserializer& reader)
{
	bool hasPrimitive{};
	reader.DeserializeVar("hasPrimitive", &hasPrimitive);
	if (!hasPrimitive)
		return;

	otherPrimitive = Primitive2DHolder{};
	otherPrimitive->Deserialize(reader);
}

void ButtonComponent::SwapPrimitive()
{
	if (!otherPrimitive.has_value())
		return;

	if (auto spriteComp{ ecs::GetEntity(this)->GetComp<SpriteComponent>() })
	{
		Primitive2DHolder prevPrimitive{ spriteComp->GetPrimitive() };
		spriteComp->SetPrimitive(*otherPrimitive);
		otherPrimitive = std::move(prevPrimitive);
	}
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
	// Reset sprites on all buttons when released
	bool wasPressed{ buttonComp.GetIsPressed() };
	if (released)
		buttonComp.ResetPressState();

	// Test if the mouse position is within the button boundaries
	if (!TestClicked(rectTransform, spriteComp, pos))
		return;

	// If pressed on button, change sprite
	if (pressed)
		buttonComp.OnPressed();
	// If released on the button that was pressed, execute button click function
	else if (released && wasPressed)
		buttonComp.OnClicked();
}
