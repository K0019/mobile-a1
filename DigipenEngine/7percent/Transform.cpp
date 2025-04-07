/******************************************************************************/
/*!
\file   Transform.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements the Transform class that stores an entity's
  2D transform values.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Transform.h"
#include "EditorHistory.h"

Transform::Transform()
	: position{}
	, posZ{ 0.1f }
	, rotation{ 0.0f }
	, scale{ 1.0f, 1.0f }
	, isTransformDirty{ false }
	, mat{}
	, parent{ nullptr }
	, children{}
{
}

Transform::Transform(const Transform& copy)
	: position{ copy.position }
	, posZ{ copy.posZ }
	, rotation{ copy.rotation }
	, scale{ copy.scale }
	, isTransformDirty{ true }
	, mat{}
	, parent{ copy.parent }
	, children{}
{
	if (parent)
		parent->children.insert(this);
}

Transform::~Transform()
{
	if (parent)
		parent->children.erase(this);
	// Inform children that we're dead
	for (Transform* child : children)
		child->SetParent(nullptr, false);
}

Transform& Transform::operator=(const Transform& copy)
{
	SetParent(copy.parent);
	// Any children we have will be carried over.

	SetLocal(copy.GetLocalPosition(), copy.GetLocalScale(), copy.GetLocalRotation());
	posZ = copy.posZ;
	return *this;
}

void Transform::SetLocalPosition(const Vec& newPos)
{
	position = newPos;
	SetDirty();
}

void Transform::AddLocalPosition(const Vec& addPos)
{
	position += addPos;
	SetDirty();
}

void Transform::SetWorldPosition(const Vec& newPos)
{
  	if (parent)
		position = parent->GetWorldMat().Inverse().MultVec(newPos, 1.0f);
	else
		 position = newPos;
	SetDirty();
}

void Transform::AddWorldPosition(const Vec& addPos)
{
	if (parent)
		position += parent->GetWorldMat().Inverse().MultVec(addPos, 0.0f);
	else
		position += addPos;
	SetDirty();
}

void Transform::SetZPos(float z)
{
	posZ = z;
	// Mat is a 3x3 so it doesn't need to be recalculated.
	// isTransformDirty = true;
}

void Transform::AddZPos(float addZ)
{
	posZ += addZ;
	// Mat is a 3x3 so it doesn't need to be recalculated.
	// isTransformDirty = true;
}

void Transform::SetLocalRotation(float newDegrees)
{
	rotation = newDegrees;
	SetDirty();
}

void Transform::AddLocalRotation(float addDegrees)
{
	rotation += addDegrees;
	SetDirty();
}

void Transform::SetWorldRotation(float newDegrees)
{
	rotation = (parent ? newDegrees - parent->GetWorldRotation() : newDegrees);
	SetDirty();
}

void Transform::AddWorldRotation(float addDegrees)
{
	// The algorithm is the same as local rotation.
	rotation += addDegrees;
	SetDirty();
}

void Transform::SetLocalScale(const Vec& newScale)
{
	scale = newScale;
	SetDirty();
}

void Transform::AddLocalScale(const Vec& addScale)
{
	scale += addScale;
	SetDirty();
}

void Transform::SetWorldScale(const Vec& newScale)
{
	scale = (parent ? newScale / parent->GetWorldScale() : newScale);
	SetDirty();
}

void Transform::AddWorldScale(const Vec& addScale)
{
	scale += (parent ? addScale / parent->GetWorldScale() : addScale);
	SetDirty();
}

void Transform::SetLocal(const Vec& newPos, const Vec& newScale, float newDegrees)
{
	position = newPos;
	rotation = newDegrees;
	scale = newScale;
	SetDirty();
}

void Transform::SetLocal(float z, const Vec& newPos, const Vec& newScale, float newDegrees)
{
	posZ = z;
	SetLocal(newPos, newScale, newDegrees);
}

void Transform::SetLocal(const Transform& transform)
{
	SetLocal(transform.posZ, transform.position, transform.scale, transform.rotation);
}

void Transform::SetWorld(const Vec& newPos, const Vec& newScale, float newDegrees)
{
	SetWorldPosition(newPos);
	SetWorldRotation(newDegrees);
	SetWorldScale(newScale);
}

void Transform::SetWorld(float z, const Vec& newPos, const Vec& newScale, float newDegrees)
{
	posZ = z;
	SetWorld(newPos, newScale, newDegrees);
}

void Transform::SetWorld(const Transform& transform)
{
	SetWorld(transform.posZ, transform.GetWorldPosition(), transform.GetWorldScale(), transform.GetWorldRotation());
}

const Transform::Vec& Transform::GetLocalPosition() const
{
	return position;
}

float Transform::GetZPos() const
{
	return posZ;
}

float Transform::GetLocalRotation() const
{
	return rotation;
}

const Transform::Vec& Transform::GetLocalScale() const
{
	return scale;
}

Transform::Vec Transform::GetWorldPosition() const
{
	return (parent ? parent->GetWorldMat() * position : position);
}

float Transform::GetWorldRotation() const
{
	return (parent ? parent->GetWorldRotation() + rotation : rotation);
}

Transform::Vec Transform::GetWorldScale() const
{
	return (parent ? parent->GetWorldScale() * scale : scale);
}

void Transform::SetParent(Transform& parentTransform)
{
	SetParent(&parentTransform);
}
void Transform::SetParent(Transform* parentTransform)
{
	SetParent(parentTransform, true);
}

Transform* Transform::GetParent()
{
	return parent;
}

const Transform* Transform::GetParent() const
{
	return parent;
}

ecs::EntityHandle Transform::GetEntity()
{
	// Offset our address backwards by some number of bytes to get to the start of the entity class object.
	return reinterpret_cast<ecs::EntityHandle>(reinterpret_cast<ecs::internal::RawData*>(this) - ecs::internal::Entity_Internal::INTERNAL_GetTransformVarByteOffset());
}

ecs::ConstEntityHandle Transform::GetEntity() const
{
	// Offset our address backwards by some number of bytes to get to the start of the entity class object.
	return reinterpret_cast<ecs::ConstEntityHandle>(reinterpret_cast<const ecs::internal::RawData*>(this) - ecs::internal::Entity_Internal::INTERNAL_GetTransformVarByteOffset());
}

void Transform::SetMat4ToWorld(glm::mat4* outMat4) const
{
	// This updates the world matrix if dirty
	if (isTransformDirty)
		GetWorldMat();

	glm::mat4& mat4{ *outMat4 };

	// Transfer x
	// x x n n
	// x x n n
	// n n n n
	// n n n n
	mat4[0][0] = mat[0][0];
	mat4[0][1] = mat[1][0];
	mat4[1][0] = mat[0][1];
	mat4[1][1] = mat[1][1];

	// Transfer y
	// x x n y
	// x x n y
	// n n n n
	// y y n n
	mat4[3][0] = mat[0][2];
	mat4[3][1] = mat[1][2];
	mat4[0][3] = mat[2][0];
	mat4[1][3] = mat[2][1];

	// Fill z
	// x x z y
	// x x z y
	// z z n n
	// y y z n
	mat4[2][0] = mat4[2][1] = mat4[2][3] =
		mat4[0][2] = mat4[1][2] = 0.0f;

	// Fill w with z position
	// x x z y
	// x x z y
	// z z n w
	// y y z n
	// Fill remaining n
	mat4[3][2] = posZ;
	mat4[2][2] = 1.0f;
	mat4[3][3] = mat[2][2];
}

void Transform::EditorDraw()
{
	gui::SetStyleVar styleFramePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 4.0f, 2.0f } };
	gui::SetStyleVar styleItemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 4.0f, 2.0f } };
	gui::Indent indent{ 4.0f };

	// Helper function for drawing the controls
	const auto DrawVec2Control = [](const char* label, Vector2* values, float columnWidth, float speed, const char* format) -> bool {
		bool modified = false;
		if (gui::Table table{ label, 3, true, gui::FLAG_TABLE::HIDE_HEADER })
		{
			table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_FIXED, columnWidth); // Set first column as fixed width. The rest will be stretch columns.
			table.SubmitColumnHeaders();

			gui::TextUnformatted(label);
			table.NextColumn();

			const auto DrawFloatComponent{ [&modified, speed, format](const char* text, const char* label, float* value, const gui::Vec4& textColor) -> void {
				{
					gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, textColor };
					gui::TextUnformatted(text);
				}
				gui::SameLine();
				gui::SetNextItemWidth(gui::GetAvailableContentRegion().x);
				modified |= gui::VarDrag(label, value, speed, 0.0f, 0.0f, format);
			} };

			DrawFloatComponent("X", "##X", &values->x, gui::Vec4{ 1.0f, 0.4f, 0.4f, 1.0f });
			table.NextColumn();
			DrawFloatComponent("Y", "##Y", &values->y, gui::Vec4{ 0.4f, 1.0f, 0.4f, 1.0f });
		}
		return modified;
	};
	const auto DrawFloatControl = [](const char* label, float* value, float columnWidth, float speed, float min, float max, const char* format) -> bool {
		if (gui::Table table{ label, 2, true, gui::FLAG_TABLE::HIDE_HEADER })
		{
			table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_FIXED, columnWidth);
			table.SubmitColumnHeaders();

			gui::TextUnformatted(label);
			table.NextColumn();

			gui::SetNextItemWidth(gui::GetAvailableContentRegion().x);
			return gui::VarDrag("##Value", value, speed, min, max, format);
		}
		return false;
	};

	Vector2 tempVec{ position };
	float tempFloat{ rotation };

	// Position Control
	if (DrawVec2Control("Position", &tempVec, 60.0f, 10.0f, "%.1f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ GetEntity(), position });
		SetLocalPosition(tempVec);
	}

	// Rotation Control
	if (DrawFloatControl("Rotation", &tempFloat, 60.0f, 1.0f, 0.0f, 360.0f, "%.1f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Rotation{ GetEntity(), rotation });
		SetLocalRotation(tempFloat);
	}

	// Scale Control
	tempVec = scale;
	if (DrawVec2Control("Scale", &tempVec, 60.0f, 10.0f, "%.2f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Scale{ GetEntity(), scale });
		SetLocalScale(tempVec);
	}
	// Z Position Control
	tempFloat = posZ;
	if (DrawFloatControl("Z Position", &tempFloat, 60.0f, 0.01f, 0.0f, 1.0f, "%.2f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_ZPos{ GetEntity(), posZ });
		SetZPos(tempFloat);
	}
}

void Transform::SetDirty()
{
	isTransformDirty = true;
	for (Transform* child : children)
		child->SetDirty();
}

const Transform::Mat& Transform::GetWorldMat() const
{
	if (isTransformDirty)
	{
		mat.SetToTransform(position, scale, rotation);
		if (parent)
			mat = parent->GetWorldMat() * mat;
		isTransformDirty = false;
	}
	return mat;
}

void Transform::SetParent(Transform* parentTransform, bool informOldParent)
{
	if (parent == parentTransform)
		return;

	// Save current world values to modify local values later so our world values don't move after parenting
	Vec worldPos{ GetWorldPosition() };
	float worldRot{ GetWorldRotation() };
	Vec worldScale{ GetWorldScale() };

	if (parent && informOldParent)
		parent->children.erase(this);
	parent = parentTransform;
	if (parent)
		parent->children.insert(this);

	// Set our local values relative to the new parent to get to our previous world values
	SetWorld(worldPos, worldScale, worldRot);
}

std::set<Transform*>& Transform::GetChildren()
{
	return children;
}
const std::set<Transform*>& Transform::GetChildren() const
{
	return children;
}

std::set<Transform*> Transform::GetChildrenRecursive()
{
	std::set<Transform*> allChildren = children;

	for (auto child : children)
	{
		std::set<Transform*> grandChildren = child->GetChildrenRecursive();
		allChildren.insert(grandChildren.begin(), grandChildren.end());
	}

	return allChildren;
}
