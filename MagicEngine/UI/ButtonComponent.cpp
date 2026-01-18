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
#include "Utilities/Logging.h"
#include <cstdio>

#include "core/platform/platform.h"

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

	// DEBUG: Log input state for diagnosing game executable input issues
	static int frameCounter = 0;
	static bool lastPressed = false;
	if (pressed != lastPressed || (frameCounter++ % 300 == 0)) {  // Log every 5 seconds or on change
		Vec2 rawPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
		printf("[ButtonInput] pressed=%d released=%d rawMousePos=(%.1f, %.1f) displaySize=(%dx%d)\n",
			pressed, released, rawPos.x, rawPos.y, Core::Display().GetWidth(), Core::Display().GetHeight());
		fflush(stdout);
		lastPressed = pressed;
	}

	if (!(pressed || released))
		return false;
	pos = RetrieveMousePos();
	// pos returns window position. Compensate for wrong aspect ratios to fix click area not aligning with button rendering
	pos.x = pos.x / static_cast<float>(Core::Display().GetWidth()) * 1920.0f;
	pos.y = pos.y / static_cast<float>(Core::Display().GetHeight()) * 1080.0f;

	printf("[ButtonInput] Click detected! scaledPos=(%.1f, %.1f)\n", pos.x, pos.y);
	fflush(stdout);

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
	if (released) {
		buttonComp.ResetPressState();
		#if defined(__ANDROID__)
		AndroidInputBridge::Release(TouchOwner::UI);
		#endif
	}
	// Test if the mouse position is within the button boundaries
	bool hitTest = internal::TestClicked(rectTransform, spriteComp, pos);

	// DEBUG: Log button hit testing
	static int buttonCheckCount = 0;
	if (buttonCheckCount++ < 5 || (pressed && !hitTest)) {  // Log first few or missed clicks
		printf("[ButtonInput] CheckButtonInput: pos=(%.1f, %.1f) hitTest=%d pressed=%d wasPressed=%d\n",
			pos.x, pos.y, hitTest, pressed, wasPressed);
		fflush(stdout);
	}

	if (!hitTest)
		return;

	// If pressed on button, change sprite
	if (pressed) {
		printf("[ButtonInput] Button pressed!\n");
		fflush(stdout);
		buttonComp.OnPressed();
		#if defined(__ANDROID__)
		AndroidInputBridge::TryCapture(TouchOwner::UI);
		#endif
	}
	// If released on the button that was pressed, execute button click function
	else if (released && wasPressed) {
		printf("[ButtonInput] Button clicked! Triggering OnClicked()\n");
		fflush(stdout);
		buttonComp.OnClicked();
	}
}
