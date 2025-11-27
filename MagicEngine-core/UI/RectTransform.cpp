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

#include "RectTransform.h"
#include "Editor/Containers/GUICollection.h"

uint16_t RectTransformComponent::GetLayer() const
{
	return static_cast<uint16_t>(ecs::GetEntityTransform(this).GetLocalPosition().z);
}
void RectTransformComponent::SetLayer(uint16_t layer)
{
	Vec3 pos{ ecs::GetEntityTransform(this).GetLocalPosition() };
	pos.z = static_cast<float>(layer) + 0.5f; // Try to avoid floating point inaccuracies with this
	ecs::GetEntityTransform(this).SetLocalPosition(pos);
}
Vec2 RectTransformComponent::GetLocalPosition() const
{
	return Vec2{ ecs::GetEntityTransform(this).GetLocalPosition() };
}
void RectTransformComponent::SetLocalPosition(Vec2 newPos)
{
	ecs::GetEntityTransform(this).SetLocalPosition(Vec3{ newPos, ecs::GetEntityTransform(this).GetLocalPosition().z });
}
Vec2 RectTransformComponent::GetWorldPosition() const
{
	return Vec2{ ecs::GetEntityTransform(this).GetWorldPosition() };
}
void RectTransformComponent::SetWorldPosition(Vec2 newPos)
{
	ecs::GetEntityTransform(this).SetWorldPosition(Vec3{ newPos, ecs::GetEntityTransform(this).GetLocalPosition().z });
}
float RectTransformComponent::GetRotation() const
{
	return ecs::GetEntityTransform(this).GetLocalRotation().x;
}
void RectTransformComponent::SetRotation(float newRot)
{
	ecs::GetEntityTransform(this).SetLocalRotation(Vec3{ newRot, 0.0f, 0.0f });
}
Vec2 RectTransformComponent::GetLocalScale() const
{
	return Vec2{ ecs::GetEntityTransform(this).GetLocalScale() };
}
Vec2 RectTransformComponent::GetWorldScale() const
{
	return Vec2{ ecs::GetEntityTransform(this).GetWorldScale() };
}
void RectTransformComponent::SetLocalScale(Vec2 newScale)
{
	ecs::GetEntityTransform(this).SetLocalScale(Vec3{ newScale, 1.0f });
}
void RectTransformComponent::SetWorldScale(Vec2 newScale)
{
	ecs::GetEntityTransform(this).SetWorldScale(Vec3{ newScale, 1.0f });
}

void RectTransformComponent::EditorDraw()
{
	unsigned int layer{ GetLayer() };
	if (gui::VarDrag("Layer", &layer))
		SetLayer(static_cast<uint16_t>(layer));
	Vec2 vec{ GetWorldPosition() };
	if (gui::VarDrag("Position", &vec))
		SetWorldPosition(vec);
	float f{ GetRotation() };
	if (gui::VarDrag("Rotation", &f))
		SetRotation(f);
	vec = GetWorldScale();
	if (gui::VarDrag("Scale", &vec))
		SetWorldScale(vec);
}

void RectTransformComponent::OnAttached()
{
	Transform& transform{ ecs::GetEntityTransform(this) };
	transform.SetLocal(
		transform.GetLocalPosition(),
		Vec3{ transform.GetLocalScale().x, transform.GetLocalScale().y, 1.0f },
		Vec3{ transform.GetLocalRotation().x, 0.0f, 0.0f }
	);
}
