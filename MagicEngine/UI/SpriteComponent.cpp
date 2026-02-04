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
const std::array<const char*, +PRIMITIVE2D_TYPE::TOTAL> Primitive2DHolder::primitiveNames{
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
int Primitive2DCircle::GetNumSegments() const
{
	return numSegments;
}

bool Primitive2DCircle::IsClicked(const RectTransformComponent& transform, Vec2 clickPos) const
{
	return ((clickPos - transform.GetWorldPosition()).LengthSqr() < radius * radius);
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

bool Primitive2DRect::IsClicked(const RectTransformComponent& transform, Vec2 clickPos) const
{
	Vec2 toClickPos{ clickPos - transform.GetWorldPosition() }, rectHalfScale{ transform.GetWorldScale() * 0.5f };
	toClickPos = glm::rotateZ(Vec3{ toClickPos, 0.0f }, -math::ToRadians(transform.GetRotation())); // Compensate for rotation
	return
		toClickPos.x >= -rectHalfScale.x && toClickPos.x <= rectHalfScale.x &&
		toClickPos.y >= -rectHalfScale.y && toClickPos.y <= rectHalfScale.y;
}

void Primitive2DRect::Render(const RectTransformComponent& transform, const Vec4& color) const
{
	Vec2 worldPos{ transform.GetWorldPosition() };
	Vec2 halfScale{ transform.GetWorldScale() * 0.5f };
	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(transform.GetLayer());
	ST<GraphicsMain>::Get()->GetImmediateGui().addSolidRect(worldPos - halfScale, worldPos + halfScale, color, drawOptions);
}

void Primitive2DRect::EditorDraw()
{
}

Primitive2DImage::Primitive2DImage()
	: uvMin{}
	, uvMax{ 1.0f, 1.0f }
{

}

void Primitive2DImage::SetImage(size_t textureHash)
{
	texture = textureHash;
}

void Primitive2DImage::SetUV(Vec2 min, Vec2 max)
{
	uvMin = min;
	uvMax = max;
}

bool Primitive2DImage::IsClicked(const RectTransformComponent& transform, Vec2 clickPos) const
{
	// TODO: Maybe provide different hit detection algos to the user
	Vec2 toClickPos{ clickPos - transform.GetWorldPosition() }, rectHalfScale{ transform.GetWorldScale() * 0.5f };
	toClickPos = glm::rotateZ(Vec3{ toClickPos, 0.0f }, -math::ToRadians(transform.GetRotation())); // Compensate for rotation
	return
		toClickPos.x >= -rectHalfScale.x && toClickPos.x <= rectHalfScale.x &&
		toClickPos.y >= -rectHalfScale.y && toClickPos.y <= rectHalfScale.y;
}

void Primitive2DImage::Render(const RectTransformComponent& transform, const Vec4& color) const
{
	const auto textureResource{ texture.GetResource() };
	if (!textureResource)
		return;

	Vec2 worldPos{ transform.GetWorldPosition() };
	Vec2 halfScale{ transform.GetWorldScale() * 0.5f };
	ui::DrawOptions drawOptions;
	drawOptions.layer = static_cast<uint16_t>(transform.GetLayer());
	ST<GraphicsMain>::Get()->GetImmediateGui().addImage(textureResource->handle, worldPos - halfScale, worldPos + halfScale, uvMin, uvMax, color, ui::SamplerMode::Linear, drawOptions);
}

void Primitive2DImage::EditorDraw()
{
	std::string textureName{ "None" };
	if (const auto textureResource{ texture.GetResource() })
		if (const auto resourceName{ ST<AssetManager>::Get()->Editor_GetName(texture.GetHash()) })
			textureName = *resourceName;
	gui::TextBoxReadOnly("Image", textureName);
	gui::PayloadTarget<size_t>("TEXTURE_HASH", [this](size_t hash) -> void {
		texture = hash;
	});

	gui::VarDrag("UV Min", &uvMin, 1.0f, Vec2{}, uvMax);
	gui::VarDrag("UV Max", &uvMax, 1.0f, uvMin, Vec2{ 1.0f, 1.0f });
}

Primitive2DHolder::Primitive2DHolder(const Primitive2D& primitive)
	: primitive{ primitive }
	, color{ 1.0f, 1.0f, 1.0f, 1.0f }
{
}

const Primitive2D& Primitive2DHolder::GetPrimitive() const
{
	return primitive;
}

const Vec4& Primitive2DHolder::GetColor() const
{
	return color;
}

void Primitive2DHolder::SetColor(const Vec4& newColor)
{
	color = newColor;
}

void Primitive2DHolder::EditorDraw()
{
	int type{ static_cast<int>(primitive.index()) };
	if (gui::Combo{ "Type", primitiveNames, &type })
		UpdatePrimitive(type);

	std::visit([](auto& primitive) -> void {
		primitive.EditorDraw();
	}, primitive);

	gui::VarColor("Color", &color);
}

void Primitive2DHolder::Serialize(Serializer& writer) const
{
	writer.Serialize("type", primitive.index());
	writer.StartObject("primitive");
	std::visit([&writer](auto& primitive) -> void {
		primitive.Serialize(writer);
		}, primitive);
	writer.EndObject();
	writer.Serialize("color", color);
}

void Primitive2DHolder::Deserialize(Deserializer& reader)
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

void Primitive2DHolder::UpdatePrimitive(size_t typeIndex)
{
	if (primitive.index() == typeIndex)
		return;
	primitive = util::VariantFromIndex<Primitive2D>(typeIndex);
}

const Primitive2DHolder& SpriteComponent::GetPrimitive() const
{
	return primitive;
}

void SpriteComponent::SetPrimitive(const Primitive2DHolder& newPrimitive)
{
	primitive = newPrimitive;
}

const Vec4& SpriteComponent::GetColor() const
{
	return primitive.GetColor();
}
void SpriteComponent::SetColor(const Vec4& newColor)
{
	primitive.SetColor(newColor);
}

void SpriteComponent::EditorDraw()
{
	primitive.EditorDraw();
}

void SpriteComponent::Serialize(Serializer& writer) const
{
	primitive.Serialize(writer);
}

void SpriteComponent::Deserialize(Deserializer& reader)
{
	primitive.Deserialize(reader);
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
