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
#include "UI/RectTransform.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

class Primitive2DBase : public ISerializeable
{
public:
	virtual void Render(const RectTransformComponent& transform, const Vec4& color) const = 0;
	virtual void EditorDraw() = 0;
};

class Primitive2DCircle : public Primitive2DBase
{
public:
	Primitive2DCircle();

	float GetRadius() const;
	void SetRadius(float newRadius);
	float GetNumSegments() const;

	void Render(const RectTransformComponent& transform, const Vec4& color) const override;
	void EditorDraw() override;
	void Deserialize(Deserializer& reader) override;

private:
	float radius;
	int numSegments;

	static int GetNumSegmentsForSmoothCircle(float radius);

public:
	property_vtable()
};
property_begin(Primitive2DCircle)
{
	property_var(radius),
	property_var(numSegments)
}
property_vend_h(Primitive2DCircle)

class Primitive2DRect : public Primitive2DBase
{
public:
	Primitive2DRect();

	void Render(const RectTransformComponent& transform, const Vec4& color) const override;
	void EditorDraw() override;

private:
	Vec2 halfDimensions;

public:
	property_vtable()
};
property_begin(Primitive2DRect)
{
	property_var(halfDimensions)
}
property_vend_h(Primitive2DRect)

class Primitive2DImage : public Primitive2DBase
{
public:
	void SetImage(size_t textureHash);

	void Render(const RectTransformComponent& transform, const Vec4& color) const override;
	void EditorDraw() override;

private:
	UserResourceHandle<ResourceTexture> texture;

public:
	property_vtable()
};
property_begin(Primitive2DImage)
{
	property_var(texture)
}
property_vend_h(Primitive2DImage)

using Primitive2D = std::variant<Primitive2DCircle, Primitive2DRect, Primitive2DImage>;

// THE ORDER OF TYPES BELOW MUST EQUAL THE ORDER OF CLASS TYPES IN THE ABOVE VARIANT
#define PRIMITIVE2D_TYPE_ENUM \
X(CIRCLE, "Circle") \
X(RECT, "Rectangle") \
X(IMAGE, "Image")
#define X(enumType, name) enumType,
enum class PRIMITIVE2D_TYPE // Note: Unused, except for "TOTAL". This is just to easily count the number of types
{
	PRIMITIVE2D_TYPE_ENUM
	TOTAL,
};
#undef X

class SpriteComponent
	: public IUIComponent
	, public IRegisteredComponent<SpriteComponent>
	, public IEditorComponent<SpriteComponent>
{
public:
	SpriteComponent();

	const Primitive2D& GetPrimitive() const;
	void VisitPrimitive(auto func);

	const Vec4& GetColor() const;
	void SetColor(const Vec4& color);

	void EditorDraw() override;

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

private:
	void UpdatePrimitive(size_t type);

private:
	Primitive2D primitive;
	Vec4 color;

	static const std::array<const char*, +PRIMITIVE2D_TYPE::TOTAL> primitiveNames;
};

class SpriteRenderSystem : public ecs::System<SpriteRenderSystem, SpriteComponent>
{
public:
	SpriteRenderSystem();

private:
	void RenderSprite(SpriteComponent& comp);
};
