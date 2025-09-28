/******************************************************************************/
/*!
\file   Physics.cpp
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   17/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief
	  This is the source file that implements the Physics system and component that
	  are defined in the interface header file.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "pch.h"
#include "Collision.h"
#include "Physics.h"
#include "NameComponent.h"

#define X(name, str) str,
static const char* physicsFlagNames[]{
	D_PHYSICS_COMP_FLAG
};
#undef X

namespace physics {
	PhysicsComp::PhysicsComp()
		: flags{ (1 << +(PHYSICS_COMP_FLAG::IS_KINEMATIC)) +
				 (1 << +(PHYSICS_COMP_FLAG::USE_GRAVITY)) }
		, bodyID {}
	{
	}

	void PhysicsComp::OnAttached()
	{
		if (auto boxColliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
		{
			bodyID = boxColliderComp->GetBodyID();
			JPH::EMotionType motionType{ GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC) ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic };
			ST<JoltPhysics>::Get()->GetBodyInterface().SetMotionType(bodyID, motionType, JPH::EActivation::Activate);
			ST<JoltPhysics>::Get()->GetBodyInterface().SetObjectLayer(bodyID, +Layers::MOVING);
		}
		else
			bodyID = ST<JoltPhysics>::Get()->CreateAndAddEmptyBody(ecs::GetEntityTransform(this), JPH::EMotionType::Dynamic, +Layers::MOVING);

		if (bodyID.IsInvalid())
			CONSOLE_LOG(LEVEL_ERROR) << "Invalid Body ID generated while creating the Physics component";
	}

	void PhysicsComp::OnDetached()
	{
		//If the entity doens't have a collider component, destroy the body from the body interface.
		if (!ecs::GetEntity(this)->HasComp<BoxColliderComp>())
			ST<JoltPhysics>::Get()->RemoveAndDestroyBody(bodyID);
		else
			ST<JoltPhysics>::Get()->GetBodyInterface().SetMotionType(bodyID, JPH::EMotionType::Static, JPH::EActivation::Activate);
	}

	JPH::BodyID PhysicsComp::GetBodyID() const
	{
		return bodyID;
	}

	bool PhysicsComp::GetFlag(PHYSICS_COMP_FLAG flag) const
	{
		return flags.TestMask(flag);
	}

	void PhysicsComp::SetFlag(PHYSICS_COMP_FLAG flag, bool val)
	{
		flags.SetMask(flag, val);
	}

	Transform const& PhysicsComp::GetPrevTransform() const
	{
		return prevTransform;
	}

	void PhysicsComp::SetPrevTransform(Transform const& transform)
	{
		prevTransform = transform;
	}

	void PhysicsComp::UpdateJoltBody()
	{
		JPH::BodyInterface& bodyInterface{ ST<JoltPhysics>::Get()->GetBodyInterface() };

		//Position
		Vec3 pos{ ecs::GetEntity(this)->GetTransform().GetWorldPosition() };
		if (pos != prevTransform.GetWorldPosition())
		{
			if (auto boxColliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				pos += boxColliderComp->GetCenter();
			bodyInterface.SetPosition(bodyID, JPH::Vec3{ pos.x, pos.y, pos.z }, JPH::EActivation::Activate);
		}

		//Scale
		Vec3 scale{ ecs::GetEntity(this)->GetTransform().GetWorldScale() };
		if (scale != prevTransform.GetWorldScale())
		{
			if (auto boxColliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
				scale *= boxColliderComp->GetSize();
			ST<JoltPhysics>::Get()->ScaleShape(bodyID, scale);
		}
		
		//Rotation
		Vec3 rot{ ecs::GetEntity(this)->GetTransform().GetWorldRotation() };
		if (rot != prevTransform.GetWorldRotation())
		{
			Vec3 radians{ math::ToRadians(rot.x), math::ToRadians(rot.y), math::ToRadians(rot.z) };
			bodyInterface.SetRotation(bodyID, JPH::QuatArg{ JPH::Quat::sEulerAngles(JPH::Vec3{radians.x, radians.y, radians.z}) }, JPH::EActivation::Activate);
		}
	}

	void PhysicsComp::UpdateTransform()
	{
		JPH::BodyInterface& bodyInterface{ ST<JoltPhysics>::Get()->GetBodyInterface() };

		//Position
		JPH::Vec3 pos{ bodyInterface.GetPosition(bodyID) };
		Vec3 center{};
		if (auto boxColliderComp{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
			center = boxColliderComp->GetCenter();
		ecs::GetEntity(this)->GetTransform().SetWorldPosition(Transform::Vec{ pos.GetX() - center.x, pos.GetY() - center.y, pos.GetZ() - center.z });

		//Rotation
		JPH::Vec3 rot{ bodyInterface.GetRotation(bodyID).GetEulerAngles()};
		Vec3 degrees{ math::ToDegrees(rot.GetX()), math::ToDegrees(rot.GetY()), math::ToDegrees(rot.GetZ()) };
		ecs::GetEntity(this)->GetTransform().SetWorldRotation(degrees);
	}

	void PhysicsComp::EditorDraw()
	{
		bool kinematic = GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC);
		if (gui::Checkbox("Is Kinematic", &kinematic))
		{
			SetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC, kinematic);

			//Change the motion type in jolt body.
			ST<JoltPhysics>::Get()->GetBodyInterface().SetMotionType(bodyID, (kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic), JPH::EActivation::Activate);
		}

		bool gravity = GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY);
		if (gui::Checkbox("Use Gravity", &gravity))
		{
			SetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY, gravity);

			//Change the gravity factor in jolt body.
			ST<JoltPhysics>::Get()->GetBodyInterface().SetGravityFactor(bodyID, (GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY) ? 1.f : 0.f));
		}
	}

	void PhysicsComp::Serialize(Serializer& writer) const
	{
		flags.MaskSerialize(writer, "flags", physicsFlagNames);
	}

	void PhysicsComp::Deserialize(Deserializer& reader)
	{
		flags.MaskDeserialize(reader, "flags", physicsFlagNames);
	}

	void PhysicsSystem::OnAdded()
	{
		// Optional step: Before starting the physics simulation you can optimize the broad phase. 
		// This improves collision detection performance.
		ST<JoltPhysics>::Get()->OptimizeBroadPhase();
	}

	bool PhysicsSystem::PreRun()
	{
		// TODO: This is kinda inefficient, if 500 boxes are physics enabled, we're gonna be doing 1000 iterations and 500 lookups just to update 500 times
		
		// In case the body's transform was changed during the stimulation using the inspector menu.
		for (auto compIter{ ecs::GetCompsActiveBegin<PhysicsComp>() }, endIter{ ecs::GetCompsEnd<PhysicsComp>() }; compIter != endIter; ++compIter)
			compIter->UpdateJoltBody();

		for (auto compIter{ ecs::GetCompsActiveBegin<BoxColliderComp>() }, endIter{ ecs::GetCompsEnd<BoxColliderComp>() }; compIter != endIter; ++compIter)
			if (!compIter.GetEntity()->HasComp<PhysicsComp>())
				compIter->UpdateJoltBody();

		// Update all the bodies.
		ST<JoltPhysics>::Get()->UpdatePhysicsSystem();

		// Update each entity based on the physics system update.
		for (auto compIter{ ecs::GetCompsActiveBegin<PhysicsComp>() }, endIter{ ecs::GetCompsEnd<PhysicsComp>() }; compIter != endIter; ++compIter)
		{
			compIter->UpdateTransform();
			compIter->SetPrevTransform(compIter.GetEntity()->GetTransform());
		}

		for (auto compIter{ ecs::GetCompsActiveBegin<BoxColliderComp>() }, endIter{ ecs::GetCompsEnd<BoxColliderComp>() }; compIter != endIter; ++compIter)
			if (!compIter.GetEntity()->HasComp<PhysicsComp>())
				compIter->SetPrevTransform(compIter.GetEntity()->GetTransform());

		return true;
	}
}