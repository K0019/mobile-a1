/******************************************************************************/
/*!
\file   Collision.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	Contains definition of member functions for collision layers and broad phase
	collision layers. It also contains the definition of the collision listener
	of Jolt Physics and the collision component member functions.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Collision.h"
#include "Physics.h"

namespace physics {
	bool ObjectLayerPairFilterImpl::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
	{
		switch (inObject1)
		{
		case +Layers::NON_MOVING:
			return +inObject2 == +Layers::MOVING; // Non moving only collides with moving
		case +Layers::MOVING:
			return +inObject2 != +Layers::NON_COLLIDABLE; // Moving collides with everything except non collidable
		case +Layers::NON_COLLIDABLE:
			return false; // Can't collide with anything.
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	BPLayerInterfaceImpl::BPLayerInterfaceImpl()
	{
		mObjectToBroadPhase[+Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
		mObjectToBroadPhase[+Layers::MOVING] = BroadPhaseLayers::MOVING;
		mObjectToBroadPhase[+Layers::NON_COLLIDABLE] = BroadPhaseLayers::NON_COLLIDABLE;
	}

	JPH::uint BPLayerInterfaceImpl::GetNumBroadPhaseLayers() const
	{
		return BroadPhaseLayers::NUM_LAYERS;
	}

	JPH::BroadPhaseLayer BPLayerInterfaceImpl::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
	{
		JPH_ASSERT(+inLayer < +Layers::NUM_LAYERS);
		return mObjectToBroadPhase[inLayer];
	}


	// TODO could not get this to compile. need to revisit later.
	/*const char* BPLayerInterfaceImpl::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
	{
		switch (static_cast<JPH::BroadPhaseLayer::Type>(inLayer))
		{
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::NON_MOVING):
			return "NON_MOVING";
		case static_cast<JPH::BroadPhaseLayer::Type>(BroadPhaseLayers::MOVING):
			return "MOVING";
		default:
			JPH_ASSERT(false);
			return "INVALID";
		}
	}*/

	bool ObjectVsBroadPhaseLayerFilterImpl::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
	{
		switch (inLayer1)
		{
		case +Layers::NON_MOVING:
			return inLayer2 == BroadPhaseLayers::MOVING;
		case +Layers::MOVING:
			return inLayer2 != BroadPhaseLayers::NON_COLLIDABLE;
		case +Layers::NON_COLLIDABLE:
			return false;
		default:
			JPH_ASSERT(false);
			return false;
		}
	}

	void MyContactListener::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& inManifold, JPH::ContactSettings& ioSettings)
	{
		CONSOLE_LOG(LEVEL_INFO) << "A contact was added";
	}

	void MyContactListener::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
	{
		CONSOLE_LOG(LEVEL_INFO) << "A contact was removed";
	}

	BoxColliderComp::BoxColliderComp()
		: bodyID{}
		, center{}
		, size{1.f, 1.f, 1.f}
	{
	}

	void BoxColliderComp::OnAttached()
	{
		//If the entity has the physics component, get the body pointer of the physics component.
		//If not, create a body.
		if (auto physComp{ ecs::GetEntity(this)->GetComp<PhysicsComp>() })
			bodyID = physComp->GetBodyID();
		else
			bodyID = ST<JoltPhysics>::Get()->CreateAndAddEmptyBody(ecs::GetEntityTransform(this), JPH::EMotionType::Static, +Layers::NON_MOVING);

		if (bodyID.IsInvalid())
			CONSOLE_LOG(LEVEL_ERROR) << "Invalid Body ID generated while creating the Physics component";

		//Set the body's shape.
		Vec3 scale{ ecs::GetEntityTransform(this).GetWorldScale() * size };
		JPH::Vec3 scaleJolt{ scale.x * 0.5f, scale.y * 0.5f, scale.z * 0.5f };

		//If the scale of any axis is 0, set it to non collidable.
		if (JPH::ScaleHelpers::IsZeroScale(scaleJolt))
		{
			scaleJolt = JPH::ScaleHelpers::MakeNonZeroScale(scaleJolt);
			ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +Layers::NON_COLLIDABLE);
		}
		
		ST<JoltPhysics>::Get()->GetBodyInterface().SetShape(bodyID, new JPH::ScaledShape(new JPH::BoxShape(JPH::Vec3::sOne()), scaleJolt), true, JPH::EActivation::Activate);
		ST<JoltPhysics>::Get()->SetBodyPosition(bodyID, ecs::GetEntityTransform(this).GetWorldPosition() + center);
	}

	void BoxColliderComp::OnDetached()
	{
		if (!ecs::GetEntity(this)->HasComp<PhysicsComp>())
			ST<JoltPhysics>::Get()->RemoveAndDestroyBody(bodyID);
		else
		{
			Vec3 scale{ ecs::GetEntityTransform(this).GetWorldScale() * 0.5f };
			JPH::Vec3 scaleJolt{scale.x, scale.y, scale.z};
			ST<JoltPhysics>::Get()->GetBodyInterface().SetShape(bodyID, new JPH::EmptyShape(scaleJolt), true, JPH::EActivation::Activate);
		}
	}

	JPH::BodyID BoxColliderComp::GetBodyID()
	{
		return bodyID;
	}

	Vec3 const& BoxColliderComp::GetCenter() const
	{
		return center;
	}

	void BoxColliderComp::SetCenter(const Vec3& val)
	{
		center = val;
	}

	Vec3 const& BoxColliderComp::GetSize() const
	{
		return size;
	}

	void BoxColliderComp::SetSize(const Vec3& val)
	{
		size = val;
	}

	Transform const& BoxColliderComp::GetPrevTransform() const
	{
		return prevTransform;
	}

	void BoxColliderComp::SetPrevTransform(const Transform& transform)
	{
		prevTransform = transform;
	}

	void BoxColliderComp::UpdateJoltBody()
	{
		JPH::BodyInterface& bodyInterface{ ST<JoltPhysics>::Get()->GetBodyInterface() };

		//Position
		Vec3 pos{ ecs::GetEntityTransform(this).GetWorldPosition() };
		if (pos != prevTransform.GetWorldPosition())
		{
			pos += center;
			bodyInterface.SetPosition(bodyID, JPH::Vec3{ pos.x, pos.y, pos.z }, JPH::EActivation::Activate);
		}

		//Scale
		Vec3 scale{ ecs::GetEntityTransform(this).GetWorldScale() };
		if (scale != prevTransform.GetWorldScale())
		{
			scale *= size;
			ST<JoltPhysics>::Get()->ScaleShape(bodyID, scale);
		}

		//Rotation
		Vec3 rot{ ecs::GetEntityTransform(this).GetWorldRotation() };
		if (rot != prevTransform.GetWorldRotation())
		{
			Vec3 radians{ math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z) };
			bodyInterface.SetRotation(bodyID, JPH::QuatArg{ JPH::Quat::sEulerAngles(JPH::Vec3{radians.x, radians.y, radians.z}) }, JPH::EActivation::Activate);
		}
	}

	void BoxColliderComp::EditorDraw()
	{
		gui::SetStyleVar styleFramePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 4.0f, 2.0f } };
		gui::SetStyleVar styleItemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 4.0f, 2.0f } };
		gui::Indent indent{ 4.0f };

		// TODO: This is copied from Transform.cpp. Should unify both implementations in GUICollection in the future.
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

		//Center of collider control
		Vec3 tempVec{ center };
		if (DrawVec3Control("Center", &center, 60.f, 1.f, "%.1f"))
		{
			ST<JoltPhysics>::Get()->SetBodyPosition(bodyID, ecs::GetEntityTransform(this).GetWorldPosition() + center);
		}

		//Size of collider control
		tempVec = size;
		if (DrawVec3Control("Size", &size, 60.f, 0.02f, "%.01f"))
		{
			ST<JoltPhysics>::Get()->ScaleShape(bodyID, ecs::GetEntityTransform(this).GetWorldScale() * size);
		}
	}

}