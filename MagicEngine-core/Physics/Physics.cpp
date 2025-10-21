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
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Components/NameComponent.h"

#define X(name, str) str,
static const char* physicsFlagNames[]{
	D_PHYSICS_COMP_FLAG
};
#undef X

namespace physics {
	PhysicsComp::PhysicsComp()
		: flags{ (1 << +(PHYSICS_COMP_FLAG::IS_KINEMATIC)) +
				 (1 << +(PHYSICS_COMP_FLAG::USE_GRAVITY)) }
		, linearVel{}
	{
	}

	void PhysicsComp::OnAttached()
	{
		if (auto bodyCompPtr{ ecs::GetEntity(this)->GetComp<JoltBodyComp>() })
		{
			JPH::EMotionType motion{ GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC) ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic };
			bodyCompPtr->SetCollisionLayer(Layers::MOVING);
			bodyCompPtr->SetMotionType(motion);
		}
		else
		{
			JPH::EMotionType motion{ GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC) ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic };
			ecs::GetEntity(this)->AddCompNow<JoltBodyComp>(JoltBodyComp{motion, ShapeType::EMPTY, Layers::NON_COLLIDABLE});
		}
	}

	void PhysicsComp::OnDetached()
	{
		if (!ecs::GetEntity(this)->HasComp<JoltBodyComp>())
			return;

		//If the entity doens't have a collider component, destroy the body from the body interface.
		if (!ecs::GetEntity(this)->HasComp<BoxColliderComp>())
			ecs::GetEntity(this)->RemoveCompNow<JoltBodyComp>();
		else
		{
			JoltBodyComp* bodyCompPtr{ ecs::GetEntity(this)->GetComp<JoltBodyComp>() };
			bodyCompPtr->SetMotionType(JPH::EMotionType::Static);
			bodyCompPtr->SetCollisionLayer(Layers::NON_MOVING);
		}
	}

	bool PhysicsComp::GetFlag(PHYSICS_COMP_FLAG flag) const
	{
		return flags.TestMask(flag);
	}

	void PhysicsComp::SetFlag(PHYSICS_COMP_FLAG flag, bool val)
	{
		flags.SetMask(flag, val);
	}

	const Vec3& PhysicsComp::GetLinearVelocity() const
	{
		return linearVel;
	}

	void PhysicsComp::SetLinearVelocity(const Vec3& vel)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLinearVelocity(vel);
		linearVel = vel;
	}

	void PhysicsComp::EditorDraw()
	{
		bool kinematic = GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC);
		if (gui::Checkbox("Is Kinematic", &kinematic))
		{
			SetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC, kinematic);

			//Change the motion type in jolt body.
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetMotionType(kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
		}

		bool gravity = GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY);
		if (gui::Checkbox("Use Gravity", &gravity))
		{
			SetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY, gravity);

			//Change the gravity factor in jolt body.
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetGravityFactor(gravity ? 1.f : 0.f);
		}
	}

	void PhysicsComp::Serialize(Serializer& writer) const
	{
		ISerializeable::Serialize(writer);
		flags.MaskSerialize(writer, "flags", physicsFlagNames);
	}

	void PhysicsComp::Deserialize(Deserializer& reader)
	{
		ISerializeable::Deserialize(reader);
		flags.MaskDeserialize(reader, "flags", physicsFlagNames);
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetMotionType(GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC) ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic);
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetGravityFactor(GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY) ? 1.f : 0.f);
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
		// Fixed by combining the physics component and collider component into jolt body component.
		for (auto compIter{ ecs::GetCompsActiveBegin<JoltBodyComp>() }, endIter{ ecs::GetCompsEnd<JoltBodyComp>() }; compIter != endIter; ++compIter)
			compIter->UpdateBody();

		// Update all the bodies.
		ST<JoltPhysics>::Get()->UpdatePhysicsSystem();

		for (auto compIter{ ecs::GetCompsActiveBegin<JoltBodyComp>() }, endIter{ ecs::GetCompsEnd<JoltBodyComp>() }; compIter != endIter; ++compIter)
			compIter->UpdateEntity();

		return true;
	}

	bool OverlapSphere(std::vector<BoxColliderComp*>& outColliders, const Vec3& origin, float radius, EntityLayersMask layers)
	{
		bool result{false};
		JPH::Float3 center{ origin.x, origin.y, origin.z };
		JPH::Sphere sphere{ center, radius };

		for (auto compIter{ ecs::GetCompsActiveBegin<BoxColliderComp>() }, endIter{ ecs::GetCompsEnd<BoxColliderComp>() }; compIter != endIter; ++compIter)
		{
			if (!layers.TestMaskAll())
			{
				if (auto layerComp{ compIter.GetEntity()->GetComp<EntityLayerComponent>() })
					if (!layers.TestMask(layerComp->GetLayer()))
						continue;
				else 
					continue;
			}

			Vec3 pos{ compIter.GetEntity()->GetComp<JoltBodyComp>()->GetPosition() };
			Vec3 scale{ compIter.GetEntity()->GetComp<JoltBodyComp>()->GetScale() / 2.f};
			Vec3 min{ pos - scale }, max{ pos + scale };
			JPH::AABox colliderAABB{ JPH::Vec3{min.x, min.y, min.z}, JPH::Vec3{max.x, max.y, max.z} };
			if (sphere.Overlaps(colliderAABB))
			{
				result = true;
				outColliders.push_back(compIter.GetCompHandle());
			}
		}

		return result;
	}
}