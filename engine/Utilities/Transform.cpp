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
#include "GUICollection.h"
#include "ScriptingUtil.h"

#ifdef IMGUI_ENABLED
#include "ImGuizmo.h"
#include "CustomViewport.h"
#include "camera.h"
#include "EditorCameraBridge.h"
#include "EditorGizmoBridge.h"

#endif
Transform::Transform()
	: position{}
	, rotation{}
	, scale{ 1.0f, 1.0f, 1.0f }
	, isTransformDirty{ false }
	, mat{}
	, parent{ nullptr }
	, children{}
{
}

Transform::Transform(const Transform& copy)
	: position{ copy.position }
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
		position = Vec3{ parent->GetWorldMat().Inverse() * Vec4{ newPos, 1.0f } };
	else
		 position = newPos;
	SetDirty();
}

void Transform::AddWorldPosition(const Vec& addPos)
{
	if (parent)
		position += Vec3{ parent->GetWorldMat().Inverse() * Vec4{ addPos, 0.0f } };
	else
		position += addPos;
	SetDirty();
}

void Transform::SetLocalRotation(const Vec& newDegrees)
{
	rotation = newDegrees;
	SetDirty();

}
void Transform::AddLocalRotation(const Vec& addDegrees)
{
	rotation += addDegrees;
	SetDirty();
}

void Transform::SetWorldRotation(const Vec& newDegrees)
{
	rotation = (parent ? newDegrees - parent->GetWorldRotation() : newDegrees);
	SetDirty();
}

void Transform::AddWorldRotation(const Vec& addDegrees)
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

void Transform::SetLocal(const Vec& newPos, const Vec& newScale, const Vec& newDegrees)
{
	position = newPos;
	rotation = newDegrees;
	scale = newScale;
	SetDirty();
}

void Transform::SetLocal(const Transform& transform)
{
	SetLocal(transform.position, transform.scale, transform.rotation);
}

void Transform::SetWorld(const Vec& newPos, const Vec& newScale, const Vec& newDegrees)
{
	SetWorldPosition(newPos);
	SetWorldRotation(newDegrees);
	SetWorldScale(newScale);
}

void Transform::SetWorld(const Transform& transform)
{
	SetWorld(transform.GetWorldPosition(), transform.GetWorldScale(), transform.GetWorldRotation());
}

const Transform::Vec& Transform::GetLocalPosition() const
{
	return position;
}

const Transform::Vec& Transform::GetLocalRotation() const
{
	return rotation;
}

const Transform::Vec& Transform::GetLocalScale() const
{
	return scale;
}

Transform::Vec Transform::GetWorldPosition() const
{
	return (parent ? Vec3{ parent->GetWorldMat() * Vec4{ position, 1.0f } } : position);
}

Transform::Vec Transform::GetWorldRotation() const
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
	*outMat4 = GetWorldMat();

	//// This updates the world matrix if dirty
	//if (isTransformDirty)
	//	GetWorldMat();

	//glm::mat4& mat4{ *outMat4 };

	//// Transfer x
	//// x x n n
	//// x x n n
	//// n n n n
	//// n n n n
	//mat4[0][0] = mat[0][0];
	//mat4[0][1] = mat[1][0];
	//mat4[1][0] = mat[0][1];
	//mat4[1][1] = mat[1][1];

	//// Transfer y
	//// x x n y
	//// x x n y
	//// n n n n
	//// y y n n
	//mat4[3][0] = mat[0][2];
	//mat4[3][1] = mat[1][2];
	//mat4[0][3] = mat[2][0];
	//mat4[1][3] = mat[2][1];

	//// Fill z
	//// x x z y
	//// x x z y
	//// z z n n
	//// y y z n
	//mat4[2][0] = mat4[2][1] = mat4[2][3] =
	//	mat4[0][2] = mat4[1][2] = 0.0f;

	//// Fill w with z position
	//// x x z y
	//// x x z y
	//// z z n w
	//// y y z n
	//// Fill remaining n
	//mat4[3][2] = posZ;
	//mat4[2][2] = 1.0f;
	//mat4[3][3] = mat[2][2];
}



void Transform::EditorDraw()
{
	
//	{
//		ImGuizmo::BeginFrame();
//		ImGuizmo::SetDrawlist(ImGui::GetForegroundDrawList());
//		ImGuizmo::Enable(true);
//
//		// --- Rect over the Scene window ---
//		const char* kSceneWindowTitle = ICON_FA_GAMEPAD " Scene";
//		ImGuiWindow* sceneWin = ImGui::FindWindowByName(kSceneWindowTitle);
//
//		ImVec2 rectMin, rectSize;
//		if (sceneWin && sceneWin->WasActive) {
//			rectMin = sceneWin->InnerRect.Min;
//			rectSize = ImVec2(sceneWin->InnerRect.GetWidth(),
//				sceneWin->InnerRect.GetHeight());
//		}
//		else {
//			const ImGuiViewport* vp = ImGui::GetMainViewport();
//			rectMin = vp->Pos; rectSize = vp->Size; // fallback
//		}
//		ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);
//
//		ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
//
//
//
//		// --- Camera from bridge (must be published this frame) ---
//		glm::mat4 V(1.0f), P(1.0f);
//		bool isOrtho = false;
//		if (!EditorCam_TryGet(V, P, isOrtho)) {
//			// No camera yet -> nothing to do this frame.
//			// (Tip: ensure EditorCam_Publish is called before UI draw.)
//		}
//		else {
//			ImGuizmo::SetOrthographic(isOrtho);
//
//#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
//			// Convert Vulkan depth [0..1] to OpenGL [-1..1] for ImGuizmo:
//			P[2][2] = P[2][2] * 0.5f;
//			P[3][2] = P[3][2] * 0.5f - 0.5f;
//#endif
//
//
//			float view[16], proj[16];
//			std::memcpy(view, glm::value_ptr(V), 16 * sizeof(float));
//			std::memcpy(proj, glm::value_ptr(P), 16 * sizeof(float));
//
//			// --- Object matrix from this Transform (WORLD) ---
//			glm::mat4 M{};
//			SetMat4ToWorld(&M);
//			float object[16];
//			std::memcpy(object, glm::value_ptr(M), 16 * sizeof(float));
//
//			// --- FOR NOW: force translate so we can validate write-back ---
//			ImGuizmo::OPERATION op = EditorGizmo_Op();   // <— force
//			ImGuizmo::MODE      mode = EditorGizmo_Mode();    // WORLD/LOCAL from your bridge
//
//			// Snap (typed pointer!)
//			bool  snapEnabled = false;                            // set true to test snapping
//			float translateSnap[3] = { 1.0f, 1.0f, 1.0f };
//			float scaleSnap[3] = { 0.1f, 0.1f, 0.1f };
//			float rotateSnapDeg = 15.0f;
//
//			const float* snap = nullptr;
//			if (snapEnabled) {
//				switch (op) {
//				case ImGuizmo::TRANSLATE: snap = translateSnap; break;
//				case ImGuizmo::SCALE:     snap = scaleSnap;     break;
//				case ImGuizmo::ROTATE:    snap = &rotateSnapDeg; break;
//				default: break;
//				}
//			}
//
//			ImGuizmo::Manipulate(view, proj, op, mode, object, nullptr, snap);
//
//
//			//bool mouseInRect = ImGui::IsMouseHoveringRect(rectMin, ImVec2(rectMin.x + rectSize.x, rectMin.y + rectSize.y), false);
//			//CONSOLE_LOG(LEVEL_INFO) << "mouseInRect=" << mouseInRect; // THIS IS WROKING FINE for now
//
////#ifdef GLM_FORCE_DEPTH_ZERO_TO_ONE
////			CONSOLE_LOG(LEVEL_INFO) << "Depth remap ACTIVE";
////#else
////			CONSOLE_LOG(LEVEL_INFO) << "Depth remap INACTIVE"; // THIS ISS HAPPENING
//////#endif
//// 			bool ok = EditorCam_TryGet(V,P,isOrtho);
////			CONSOLE_LOG(LEVEL_INFO) << "GotCam=" << ok; // 1
//			//bool sceneHovered = sceneWin && ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows);
//			//CONSOLE_LOG(LEVEL_INFO) << "sceneHovered=" << sceneHovered; // returns 0
//
//
//			// --- WRITE-BACK (translate only) ---
//			if (ImGuizmo::IsUsing())
//			{
//				float t[3], r[3], s[3];
//				ImGuizmo::DecomposeMatrixToComponents(object, t, r, s);
//
//				const Transform* parent = GetParent();
//				if (mode == ImGuizmo::WORLD || !parent) {
//					SetWorldPosition({ t[0], t[1], t[2] });
//				}
//				else {
//					glm::mat4 parentWorld{}; parent->SetMat4ToWorld(&parentWorld);
//					glm::mat4 worldM = glm::make_mat4(object);
//					glm::mat4 localM = glm::inverse(parentWorld) * worldM;
//
//					ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localM), t, r, s);
//					SetLocalPosition({ t[0], t[1], t[2] });
//				}
//			}
//			else {
//				// optional: show quick hints while diagnosing
//				if (ImGuizmo::IsOver()) {
//					CONSOLE_LOG(LEVEL_INFO) << "oVer fgizmo but not dragging";
//				}
//				else {
//
//				}
//
//			}
//		}
//	}


	//=====================END GUIZMO

	gui::SetStyleVar styleFramePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 4.0f, 2.0f } };
	gui::SetStyleVar styleItemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 4.0f, 2.0f } };
	gui::Indent indent{ 4.0f };
														
	// Helper function for drawing the controls																	
	const auto DrawVec3Control = [](const char* label, Vec3* values, float columnWidth, float speed, const char* format) -> bool {
		bool modified = false;
		if (gui::Table table{ label, 4, true, gui::FLAG_TABLE::HIDE_HEADER })
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
			table.NextColumn();
			DrawFloatComponent("Z", "##Z", &values->z, gui::Vec4{ 0.4f, 0.4f, 1.0f, 1.0f });
		}
		return modified;
	};

	Vec3 tempVec{ position };

	// Position Control
	if (DrawVec3Control("Position", &tempVec, 60.0f, 10.0f, "%.1f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ GetEntity(), position });
		SetLocalPosition(tempVec);
	}

	// Rotation Control
	tempVec = rotation;
	if (DrawVec3Control("Rotation", &tempVec, 60.0f, 1.0f, "%.1f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Rotation{ GetEntity(), rotation });
		SetLocalRotation(tempVec);
	}

	// Scale Control
	tempVec = scale;
	if (DrawVec3Control("Scale", &tempVec, 60.0f, 10.0f, "%.1f"))
	{
		ST<History>::Get()->IntermediateEvent(HistoryEvent_Scale{ GetEntity(), scale });
		SetLocalScale(tempVec);
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
		mat.Set(position, scale, rotation);
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
	Vec worldRot{ GetWorldRotation() };
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


#pragma region Scripting

/*****************************************************************//*!
\brief
	Copy of the equivalent Transform structure in the C# project.
*//******************************************************************/
struct CS_Transform
{
	Transform::Vec position;
	Transform::Vec localPosition;
	Transform::Vec scale;
	Transform::Vec localScale;
	Transform::Vec rotation;
	Transform::Vec localRotation;

	ecs::EntityHandle entity;
};

/*****************************************************************//*!
\brief
	Returns the transform of an entity to C# side.
\param entity
	The entity.
\return
	The transform of the entity, in a format compatible with C#'s
	definition of a Transform.
*//******************************************************************/
SCRIPT_CALLABLE CS_Transform CS_GetTransform(ecs::EntityHandle entity)
{
	util::AssertEntityHandleValid(entity);

	Transform& t = entity->GetTransform();
	return CS_Transform{
		t.GetWorldPosition(),
		t.GetLocalPosition(),
		t.GetWorldScale(),
		t.GetLocalScale(),
		t.GetWorldRotation(),
		t.GetLocalRotation(),
		entity
	};
}

/*****************************************************************//*!
\brief
	Updates the transform structure on C# side.
\param inOutTransform
	Pointer to the transform structure on C# side.
	Member var entity will be used to search the appropriate entity.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetTransform(CS_Transform* inOutTransform)
{
	util::AssertEntityHandleValid(inOutTransform->entity);

	Transform& t = inOutTransform->entity->GetTransform();
	inOutTransform->position = t.GetWorldPosition();
	inOutTransform->localPosition = t.GetLocalPosition();
	inOutTransform->scale = t.GetWorldScale();
	inOutTransform->localScale = t.GetLocalScale();
	inOutTransform->rotation = t.GetWorldRotation();
	inOutTransform->localRotation = t.GetLocalRotation();
}

/*****************************************************************//*!
\brief
	Sets the local position of an entity.
\param entity
	The entity.
\param position
	The local position.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetLocalPosition(ecs::EntityHandle entity, Vec3 position)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetLocalPosition(position);
}

/*****************************************************************//*!
\brief
	Sets the world position of an entity.
\param entity
	The entity.
\param position
	The world position.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetWorldPosition(ecs::EntityHandle entity, Vec3 position)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetWorldPosition(position);
}

/*****************************************************************//*!
\brief
	Sets the local rotation of an entity.
\param entity
	The entity.
\param rotation
	The local rotation.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetLocalRotation(ecs::EntityHandle entity, Vec3 rotation)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetLocalRotation(rotation);
}

/*****************************************************************//*!
\brief
	Sets the world rotation of an entity.
\param entity
	The entity.
\param rotation
	The world rotation.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetWorldRotation(ecs::EntityHandle entity, Vec3 rotation)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetWorldRotation(rotation);
}

/*****************************************************************//*!
\brief
	Sets the local scale of an entity.
\param entity
	The entity.
\param scale
	The local scale.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetLocalScale(ecs::EntityHandle entity, Vec3 scale)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetLocalScale(scale);
}

/*****************************************************************//*!
\brief
	Sets the world scale of an entity.
\param entity
	The entity.
\param scale
	The world scale.
*//******************************************************************/
SCRIPT_CALLABLE void CS_SetWorldScale(ecs::EntityHandle entity, Vec3 scale)
{
	util::AssertEntityHandleValid(entity);
	entity->GetTransform().SetWorldScale(scale);
}

#pragma endregion // Scripting
