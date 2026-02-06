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
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "Utilities/Logging.h"
#include <cstdio>

#include "core/platform/platform.h"
#include "SliderComponent.h"

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

	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnButtonPressed");
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

	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnButtonReleased");
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
    // Look for any unowned pointer that just went down
    int downPid = AndroidInputBridge::FindUnownedJustDown();
    pressed = (downPid >= 0);

    // Check if any pointer just went up (for release detection)
    released = false;
    for (int i = 0; i < AndroidInputBridge::MAX_POINTERS; ++i) {
        if (AndroidInputBridge::State(i).justUp) {
            released = true;
            break;
        }
    }

    if (pressed) {
        const auto& ts = AndroidInputBridge::State(downPid);
        m_activePid = downPid;
        // Convert to UI space
        Vec2 rawPos{ ts.x, ts.y };
        float uiX, uiY;
        if (!ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY))
            return false;
        pos = Vec2{ uiX, uiY };
        return true;
    }
    else if (released) {
        // Find a pointer that just went up and get its position
        for (int i = 0; i < AndroidInputBridge::MAX_POINTERS; ++i) {
            if (AndroidInputBridge::State(i).justUp) {
                const auto& ts = AndroidInputBridge::State(i);
                Vec2 rawPos{ ts.x, ts.y };
                float uiX, uiY;
                if (ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY)) {
                    pos = Vec2{ uiX, uiY };
                    return true;
                }
            }
        }
        return false;
    }
    return false;
#else
	pressed = ST<KeyboardMouseInput>::Get()->GetIsPressed(KEY::M_LEFT);
	released = ST<KeyboardMouseInput>::Get()->GetIsReleased(KEY::M_LEFT);

	static int frameCounter = 0;
	static bool lastPressed = false;
	if (pressed != lastPressed || (frameCounter++ % 300 == 0)) {
		Vec2 rawPos = ST<KeyboardMouseInput>::Get()->GetMousePos();
		printf("[ButtonInput] pressed=%d released=%d rawMousePos=(%.1f, %.1f) displaySize=(%dx%d)\n",
			pressed, released, rawPos.x, rawPos.y, Core::Display().GetWidth(), Core::Display().GetHeight());
		fflush(stdout);
		lastPressed = pressed;
	}

	if (!(pressed || released))
		return false;

	Vec2 rawPos = MagicInput::GetMousePos();
	float uiX, uiY;
	if (!ST<GraphicsMain>::Get()->WindowToUIPosition(rawPos.x, rawPos.y, uiX, uiY))
		return false;
	pos = Vec2{ uiX, uiY };
	return true;
#endif
}

void ButtonInputSystem::CheckButtonInput(ButtonComponent& buttonComp, SpriteComponent& spriteComp, RectTransformComponent& rectTransform)
{
	bool wasPressed{ buttonComp.GetIsPressed() };
	if (released) {
		buttonComp.ResetPressState();
		#if defined(__ANDROID__)
		AndroidInputBridge::Release(TouchOwner::UI);
		#endif
	}

	bool hitTest = internal::TestClicked(rectTransform, spriteComp, pos);

	if (!hitTest)
		return;

	if (pressed) {
		buttonComp.OnPressed();
		#if defined(__ANDROID__)
		AndroidInputBridge::TryCapture(TouchOwner::UI, m_activePid);
		#endif
	}
	else if (released && wasPressed) {
		buttonComp.OnClicked();
	}
}
