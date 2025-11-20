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
#include "Editor/IEditorComponent.h"
#include "UI/IUIComponent.h"

class TextComponent
	: public IUIComponent
	, public IRegisteredComponent<TextComponent>
	, public IEditorComponent<TextComponent>
{
public:
	TextComponent();

	const std::string& GetText() const;
	void SetText(const std::string& newText);
	const Vec4& GetColor() const;
	void SetColor(const Vec4& color);

	void EditorDraw() override;

private:
	std::string text;
	Vec4 color;

public:
	property_vtable()
};
property_begin(TextComponent)
{
	property_var(text),
	property_var(color)
}
property_vend_h(TextComponent)

class TextSystem : public ecs::System<TextSystem, TextComponent>
{
public:
	TextSystem();

private:
	void RenderText(TextComponent& comp);
};
