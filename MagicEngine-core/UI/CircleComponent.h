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

class CircleComponent
	: public IUIComponent
	, public IRegisteredComponent<CircleComponent>
	, public IEditorComponent<CircleComponent>
{
public:
	CircleComponent();

	float GetRadius() const;
	void SetRadius(float newRadius);
	float GetNumSegments() const;
	const Vec4& GetColor() const;
	void SetColor(const Vec4& color);

	void EditorDraw() override;

	void Deserialize(Deserializer& reader) override;

private:
	static int GetNumSegmentsForSmoothCircle(float radius);

private:
	float radius;
	int numSegments;
	Vec4 color;

public:
	property_vtable()
};
property_begin(CircleComponent)
{
	property_var(radius),
	property_var(color)
}
property_vend_h(CircleComponent)

class CircleSystem : public ecs::System<CircleSystem, CircleComponent>
{
public:
	CircleSystem();

private:
	void RenderCircle(CircleComponent& comp);
};
