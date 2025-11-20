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

#include "CircleComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"

CircleComponent::CircleComponent()
	: radius{ 10.0f }
	, numSegments{ 8 }
	, color{ 1.0f, 1.0f, 1.0f, 1.0f }
{
}

float CircleComponent::GetRadius() const
{
	return radius;
}
void CircleComponent::SetRadius(float newRadius)
{
	radius = newRadius;
}
float CircleComponent::GetNumSegments() const
{
	return numSegments;
}
const Vec4& CircleComponent::GetColor() const
{
	return color;
}
void CircleComponent::SetColor(const Vec4& newColor)
{
	color = newColor;
}

void CircleComponent::EditorDraw()
{
	if (gui::VarDrag("Radius", &radius))
		numSegments = GetNumSegmentsForSmoothCircle(radius);
	gui::VarColor("Color", &color);
}

void CircleComponent::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);

	numSegments = GetNumSegmentsForSmoothCircle(radius);
}

int CircleComponent::GetNumSegmentsForSmoothCircle(float radius)
{
	if (radius <= 1.0f)
		return 2;

	// https://stackoverflow.com/questions/11774038/how-to-render-a-circle-with-as-few-vertices-as-possible
	float th{ std::acosf(2.0f * math::PowSqr(1.0f - 0.8f / radius) - 1.0f) };
	return static_cast<int>(std::ceilf(2.0f * math::PI_f / th) + 0.5f);
}

CircleSystem::CircleSystem()
	: System_Internal{ &CircleSystem::RenderCircle }
{
}

void CircleSystem::RenderCircle(CircleComponent& comp)
{
	const Vec3 worldPos{ ecs::GetEntityTransform(&comp).GetWorldPosition() };

	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(worldPos.z);
	ST<GraphicsMain>::Get()->GetImmediateGui().addCircleFilled(worldPos, comp.GetRadius(), comp.GetColor(), comp.GetNumSegments(), drawOptions);
}
