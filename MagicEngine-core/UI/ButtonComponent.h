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
#include "UI/SpriteComponent.h"
#include "ECS/IRegisteredComponent.h"
#include "Editor/IEditorComponent.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

class ButtonComponent
	: public IRegisteredComponent<ButtonComponent>
	, public IEditorComponent<ButtonComponent>
	, public IUIComponent<ButtonComponent>
{
public:
	ButtonComponent();

	void OnPressed();
	bool GetIsPressed() const;
	void ResetPressState();

	void OnClicked();

	// Forces attachment of a sprite component
	void OnAttached() override;

	void EditorDraw() override;
	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

private:
	void SwapPrimitive();

private:
	std::optional<Primitive2DHolder> otherPrimitive;
	bool isPressed;

};

class ButtonInputSystem : public ecs::System<ButtonInputSystem, ButtonComponent, SpriteComponent, RectTransformComponent>
{
public:
	ButtonInputSystem();
	
	bool PreRun() override;

private:
	Vec2 RetrieveMousePos();
	void CheckButtonInput(ButtonComponent& buttonComp, SpriteComponent& spriteComp, RectTransformComponent& rectTransform);

private:
	bool pressed, released;
	Vec2 pos;

};
