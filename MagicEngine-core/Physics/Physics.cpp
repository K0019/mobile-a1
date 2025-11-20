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
#include "Editor/Containers/GUICollection.h"

#define X(name, str) str,
static const char* physicsFlagNames[]{
	D_PHYSICS_COMP_FLAG
};
#undef X

namespace physics {
	PhysicsComp::PhysicsComp()
		: flags{ PHYSICS_COMP_FLAG::USE_GRAVITY, PHYSICS_COMP_FLAG::ENABLED }
	{
	}

	void PhysicsComp::OnAttached()
	{
		JPH::EMotionType motion{};
		if (!GetFlag(PHYSICS_COMP_FLAG::ENABLED))
			motion = JPH::EMotionType::Static;
		else if (GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC))
			motion = JPH::EMotionType::Kinematic;
		else
			motion = JPH::EMotionType::Dynamic;

		if (auto colCompPtr{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() })
		{
			auto bodyCompPtr{ ecs::GetEntity(this)->GetComp<JoltBodyComp>() };
			if (!bodyCompPtr || bodyCompPtr->GetBodyID().IsInvalid())
				return;

			bodyCompPtr->SetCollisionLayer(colCompPtr->GetFlag(COLLIDER_COMP_FLAG::ENABLED) ? Layers::MOVING : Layers::NON_COLLIDABLE);
			bodyCompPtr->SetGravityFactor(GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY) ? 1.f : 0.f);
			bodyCompPtr->SetMotionType(motion);
		}
		else
		{
			ecs::GetEntity(this)->AddCompNow<JoltBodyComp>(JoltBodyComp{ motion, ShapeType::EMPTY, Layers::NON_COLLIDABLE });
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
			Layers layer{Layers::NON_MOVING};
			auto colCompPtr{ ecs::GetEntity(this)->GetComp<BoxColliderComp>() };
			if (!colCompPtr->GetFlag(COLLIDER_COMP_FLAG::ENABLED))
				layer = Layers::NON_COLLIDABLE;
			bodyCompPtr->SetMotionType(JPH::EMotionType::Static);
			bodyCompPtr->SetCollisionLayer(layer);
		}
	}

	bool PhysicsComp::GetFlag(PHYSICS_COMP_FLAG flag) const
	{
		return flags.TestMask(flag);
	}

	void PhysicsComp::SetFlag(PHYSICS_COMP_FLAG flag, bool val)
	{
		flags.SetMask(flag, val);

		JPH::EMotionType type{};
		switch (flag)
		{
		case PHYSICS_COMP_FLAG::ENABLED:
			if (!val)
				type = JPH::EMotionType::Static;
			else if (GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC))
				type = JPH::EMotionType::Kinematic;
			else
				type = JPH::EMotionType::Dynamic;
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetMotionType(type);
			break;
		case PHYSICS_COMP_FLAG::IS_KINEMATIC:
			if (!GetFlag(PHYSICS_COMP_FLAG::ENABLED))
				type = JPH::EMotionType::Static;
			else if (val)
				type = JPH::EMotionType::Kinematic;
			else
				type = JPH::EMotionType::Dynamic;
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetMotionType(type);
			break;
		case PHYSICS_COMP_FLAG::USE_GRAVITY:
			//Change the gravity factor in jolt body.
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetGravityFactor(val ? 1.f : 0.f);
			break;
		case PHYSICS_COMP_FLAG::ROTATION_LOCKED_X:
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationX(val);
			break;
		case PHYSICS_COMP_FLAG::ROTATION_LOCKED_Y:
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationY(val);
			break;
		case PHYSICS_COMP_FLAG::ROTATION_LOCKED_Z:
			ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationZ(val);
			break;
		}
	}

	Vec3 PhysicsComp::GetLinearVelocity() const
	{
		return ecs::GetEntity(this)->GetComp<JoltBodyComp>()->GetLinearVelocity();
	}

	void PhysicsComp::SetLinearVelocity(const Vec3& vel)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLinearVelocity(vel);
	}

	void PhysicsComp::AddLinearVelocity(const Vec3& vel)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->AddLinearVelocity(vel);
	}

	Vec3 PhysicsComp::GetAngularVelocity() const
	{
		return ecs::GetEntity(this)->GetComp<JoltBodyComp>()->GetAngularVelocity();
	}

	void PhysicsComp::SetAngularVelocity(const Vec3& vel)
	{
		ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetAngularVelocity(vel);
	}

	void PhysicsComp::EditorDraw()
	{
		bool enabled{ GetFlag(PHYSICS_COMP_FLAG::ENABLED) };
		bool kinematic{ GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC) };
		bool lockX{ GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_X) };
		bool lockY{ GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Y) };
		bool lockZ{ GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Z) };
		if (gui::Checkbox(physicsFlagNames[+PHYSICS_COMP_FLAG::ENABLED], &enabled))
		{
			SetFlag(PHYSICS_COMP_FLAG::ENABLED, enabled);

			if (enabled)
			{
				ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationX(lockX);
				ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationY(lockY);
				ecs::GetEntity(this)->GetComp<JoltBodyComp>()->SetLockRotationZ(lockZ);
			}
		}

		if (gui::Checkbox("Is Kinematic", &kinematic))
		{
			SetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC, kinematic);
		}

		bool gravity{ GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY) };
		if (gui::Checkbox("Use Gravity", &gravity))
		{
			SetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY, gravity);
		}

		gui::TextUnformatted("Freeze Rotation");
		if (gui::Checkbox("X", &lockX))
		{
			SetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_X, lockX);
		}
		gui::SameLine();
		if (gui::Checkbox("Y", &lockY))
		{
			SetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Y, lockY);
		}
		gui::SameLine();
		if (gui::Checkbox("Z", &lockZ))
		{
			SetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Z, lockZ);
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
		if (ecs::GetCurrentPoolId() == ecs::POOL::DEFAULT)
		{
			ST<Scheduler>::Get()->Add(0.0f, [entity = ecs::GetEntity(this)]() {
				JPH::EMotionType type{};
				auto compPtr{ entity->GetComp<PhysicsComp>() };
				if (!compPtr->GetFlag(PHYSICS_COMP_FLAG::ENABLED))
					type = JPH::EMotionType::Static;
				else if (compPtr->GetFlag(PHYSICS_COMP_FLAG::IS_KINEMATIC))
					type = JPH::EMotionType::Kinematic;
				else
					type = JPH::EMotionType::Dynamic;
				if (!entity->HasComp<JoltBodyComp>())
					return;
				entity->GetComp<JoltBodyComp>()->SetMotionType(type);
				entity->GetComp<JoltBodyComp>()->SetGravityFactor(compPtr->GetFlag(PHYSICS_COMP_FLAG::USE_GRAVITY) ? 1.f : 0.f);
				entity->GetComp<JoltBodyComp>()->SetLockRotationX(compPtr->GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_X));
				entity->GetComp<JoltBodyComp>()->SetLockRotationY(compPtr->GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Y));
				entity->GetComp<JoltBodyComp>()->SetLockRotationZ(compPtr->GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED_Z));
				});
		}
	}

	void PhysicsSystem::OnAdded()
	{
		// Optional step: Before starting the physics simulation you can optimize the broad phase. 
		// This improves collision detection performance.
		ST<JoltPhysics>::Get()->OptimizeBroadPhase();
	}

	bool PhysicsSystem::PreRun()
	{	
		for (auto compIter{ ecs::GetCompsActiveBegin<JoltBodyComp>() }, endIter{ ecs::GetCompsEnd<JoltBodyComp>() }; compIter != endIter; ++compIter)
			compIter->UpdateBody();

		MyContactListener::ClearContactPair();

		// Update all the bodies.
		ST<JoltPhysics>::Get()->UpdatePhysicsSystem();

		MyContactListener::CallContactFunc();

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
				{
					if (!layers.TestMask(layerComp->GetLayer()))
						continue;
				}
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

	bool OverlapBox(std::vector<BoxColliderComp*>& outColliders, const Vec3& origin, const Vec3& halfExtent, const Vec3& orientation, EntityLayersMask layers)
	{
		bool result{ false };
		Vec3 boxMin{ origin - halfExtent }, boxMax{ origin + halfExtent };
		JPH::AABox box{ JPH::Vec3{boxMin.x, boxMin.y, boxMin.z}, JPH::Vec3{boxMax.x, boxMax.y, boxMax.z} };

		for (auto compIter{ ecs::GetCompsActiveBegin<BoxColliderComp>() }, endIter{ ecs::GetCompsEnd<BoxColliderComp>() }; compIter != endIter; ++compIter)
		{
			if (!layers.TestMaskAll())
			{
				if (auto layerComp{ compIter.GetEntity()->GetComp<EntityLayerComponent>() })
				{
					if (!layers.TestMask(layerComp->GetLayer()))
						continue;
				}
				else
					continue;
			}

			Vec3 pos{ compIter.GetEntity()->GetComp<JoltBodyComp>()->GetPosition() };
			Vec3 scale{ compIter.GetEntity()->GetComp<JoltBodyComp>()->GetScale() / 2.f };
			Vec3 min{ pos - scale }, max{ pos + scale };
			JPH::AABox colliderAABB{ JPH::Vec3{min.x, min.y, min.z}, JPH::Vec3{max.x, max.y, max.z} };
			if (box.Overlaps(colliderAABB))
			{
				result = true;
				outColliders.push_back(compIter.GetCompHandle());
			}
		}

		return result;
	}
}