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

#include "SpriteComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"

#define X(enumType, name) name,
const std::array<const char*, +PRIMITIVE2D_TYPE::TOTAL> SpriteComponent::primitiveNames{
	PRIMITIVE2D_TYPE_ENUM
};

Primitive2DCircle::Primitive2DCircle()
	: radius{ 10.0f }
	, numSegments{ 8 }
{
}

float Primitive2DCircle::GetRadius() const
{
	return radius;
}
void Primitive2DCircle::SetRadius(float newRadius)
{
	radius = newRadius;
	numSegments = GetNumSegmentsForSmoothCircle(radius);
}
float Primitive2DCircle::GetNumSegments() const
{
	return numSegments;
}

void Primitive2DCircle::Render(const RectTransformComponent& transform, const Vec4& color) const
{
	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(transform.GetLayer());
	ST<GraphicsMain>::Get()->GetImmediateGui().addCircleFilled(transform.GetWorldPosition(), radius, color, numSegments, drawOptions);
}

void Primitive2DCircle::EditorDraw()
{
	if (gui::VarDrag("Radius", &radius))
		numSegments = GetNumSegmentsForSmoothCircle(radius);
}

void Primitive2DCircle::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);
	numSegments = GetNumSegmentsForSmoothCircle(radius);
}

int Primitive2DCircle::GetNumSegmentsForSmoothCircle(float radius)
{
	if (radius <= 1.0f)
		return 2;

	// https://stackoverflow.com/questions/11774038/how-to-render-a-circle-with-as-few-vertices-as-possible
	float th{ std::acosf(2.0f * math::PowSqr(1.0f - 0.8f / radius) - 1.0f) };
	return static_cast<int>(std::ceilf(2.0f * math::PI_f / th) + 0.5f);
}

Primitive2DRect::Primitive2DRect()
	: halfDimensions{ 5.0f, 5.0f }
{
}

Vec2 Primitive2DRect::GetDimensions() const
{
	return halfDimensions * 2.0f;
}

void Primitive2DRect::SetDimensions(Vec2 newDimensions)
{
	halfDimensions = newDimensions * 0.5f;
}

void Primitive2DRect::Render(const RectTransformComponent& transform, const Vec4& color) const
{
	Vec2 worldPos{ transform.GetWorldPosition() };
	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(transform.GetLayer());
	ST<GraphicsMain>::Get()->GetImmediateGui().addSolidRect(worldPos - halfDimensions, worldPos + halfDimensions, color, drawOptions);
}

void Primitive2DRect::EditorDraw()
{
	Vec2 dimensions{ GetDimensions() };
	if (gui::VarDrag("Dimensions", &dimensions))
		SetDimensions(dimensions);
}

SpriteComponent::SpriteComponent()
	: primitive{ Primitive2DCircle{} }
	, color{ 1.0f, 1.0f, 1.0f, 1.0f }
{
}

const Primitive2D& SpriteComponent::GetPrimitive() const
{
	return primitive;
}

void SpriteComponent::VisitPrimitive(auto func)
{
	std::visit(func, primitive);
}

const Vec4& SpriteComponent::GetColor() const
{
	return color;
}
void SpriteComponent::SetColor(const Vec4& newColor)
{
	color = newColor;
}

void SpriteComponent::EditorDraw()
{
	int type{ static_cast<int>(primitive.index()) };
	if (gui::Combo{ "Type", primitiveNames, &type })
		UpdatePrimitive(type);

	std::visit([](auto& primitive) -> void {
		primitive.EditorDraw();
	}, primitive);

	gui::VarColor("Color", &color);
}

void SpriteComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("type", primitive.index());
	writer.StartObject("primitive");
	std::visit([&writer](auto& primitive) -> void {
		primitive.Serialize(writer);
	}, primitive);
	writer.EndObject();
	writer.Serialize("color", color);
}

void SpriteComponent::Deserialize(Deserializer& reader)
{
	size_t type{};
	reader.DeserializeVar("type", &type);
	UpdatePrimitive(type);
	if (reader.PushAccess("primitive"))
	{
		std::visit([&reader](auto& primitive) -> void {
			primitive.Deserialize(reader);
		}, primitive);
		reader.PopAccess();
	}
	reader.DeserializeVar("color", &color);
}

void SpriteComponent::UpdatePrimitive(size_t typeIndex)
{
	if (primitive.index() == typeIndex)
		return;
	primitive = util::VariantFromIndex<Primitive2D>(typeIndex);
}

SpriteRenderSystem::SpriteRenderSystem()
	: System_Internal{ &SpriteRenderSystem::RenderSprite }
{
}

void SpriteRenderSystem::RenderSprite(SpriteComponent& comp)
{
	comp.VisitPrimitive([&comp](auto& primitive) -> void {
		ecs::CompHandle<RectTransformComponent> rectTransform{ ecs::GetEntity(&comp)->GetComp<RectTransformComponent>() };
		primitive.Render(*rectTransform, comp.GetColor());
	});
}
