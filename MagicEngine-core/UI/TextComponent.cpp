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

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "TextComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"

TextComponent::TextComponent()
	: text{}
	, color{ 1.0f, 1.0f, 1.0f, 1.0f }
{
}

const std::string& TextComponent::GetText() const
{
	return text;
}
void TextComponent::SetText(const std::string& newText)
{
	text = newText;
}
const Vec4& TextComponent::GetColor() const
{
	return color;
}
void TextComponent::SetColor(const Vec4& newColor)
{
	color = newColor;
}

void TextComponent::EditorDraw()
{
	gui::VarDefault("Text", &text);
	gui::VarColor("Color", &color);
}

TextSystem::TextSystem()
	: System_Internal{ &TextSystem::RenderText }
{
}

void TextSystem::RenderText(TextComponent& comp)
{
	const Vec3 worldPos{ ecs::GetEntityTransform(&comp).GetWorldPosition() };

	ui::TextLayoutDesc layoutDesc;
	layoutDesc.origin = worldPos;
	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(worldPos.z);
	ST<GraphicsMain>::Get()->GetImmediateGui().addText(comp.GetText(), layoutDesc, comp.GetColor(), drawOptions);
}
