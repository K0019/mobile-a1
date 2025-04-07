/******************************************************************************/
/*!
\file   Physics.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file that implements the Physics system and component that
  are defined in the interface header file.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Physics.h"
#include "Collision.h"
#include "GameSettings.h"
#include "Editor.h"

#define X(name, str) str,
static const char* physicsFlagNames[]{
	D_PHYSICS_COMP_FLAG
};
#undef X

namespace Physics
{

#pragma region PhysicsComp

	PhysicsComp::PhysicsComp(const PhysicsCompParams& params) :
#ifdef IMGUI_ENABLED
		REGISTER_DRAW_FUNCTION_TO_EDITOR(EditorDraw),
#endif
		flags{
			(static_cast<int>(params.isDynamic) << static_cast<int>(PHYSICS_COMP_FLAG::IS_DYNAMIC)) +
			(1 << static_cast<int>(PHYSICS_COMP_FLAG::ENABLE_GRAVITY))
	}
		, mass{ params.mass }
		, restitutionCoeff{ params.restitutionCoeff }
		, frictionCoeff{ params.frictionCoeff }
		, velocity{}
		, angVelocity{ 0.0f }
	{
	}

	void PhysicsComp::ApplyImpulse(const Vector2& impulse, const Vector2& relativePoint, float momentOfInertia, bool canRotate)
	{
		UNREFERENCED_PARAMETER(momentOfInertia);
		UNREFERENCED_PARAMETER(relativePoint);

		if (!IsDynamic())
			return;

		AddVelocity(impulse * GetMassReciprocal());
		if (canRotate && !IsRotationLocked())
			// DIRTY FIX REMOVE WHEN PROJECT ENDS!
			// Only gibs want rotation. They can stop rotating once they collide with something.
			SetAngVelocity(0.0f);
			//AddAngVelocity(math::ToDegrees(relativePoint.CrossScalar(impulse) * (1.0f / momentOfInertia)));
	}

	const Vector2& PhysicsComp::GetVelocity() const
	{
		return velocity;
	}
	void PhysicsComp::SetVelocity(const Vector2& newVel)
	{
		velocity = newVel;
	}
	void PhysicsComp::AddVelocity(const Vector2& addVel)
	{
		velocity += addVel;
	}

	float PhysicsComp::GetAngVelocity() const
	{
		return angVelocity;
	}
	void PhysicsComp::SetAngVelocity(float newAngVel)
	{
		if (!IsRotationLocked())
			angVelocity = newAngVel;
	}
	void PhysicsComp::AddAngVelocity(float addAngVel)
	{
		if (!IsRotationLocked())
			angVelocity += addAngVel;
	}

	float PhysicsComp::GetMass() const
	{
		// If collider is static, calculations should assume it has infinite mass.
		return IsDynamic() ? mass : std::numeric_limits<float>::infinity();
	}

	float PhysicsComp::GetMassReciprocal() const
	{
		// If collider is static, calculations should assume it has infinite mass -> 1/mass == 0
		return IsDynamic() ? 1.0f / mass : 0.0f;
	}

	float PhysicsComp::GetRestitutionCoeff() const
	{
		if (!IsDynamic())
			return 0.0f;
		return restitutionCoeff;
	}

	float PhysicsComp::GetFrictionCoeff() const
	{
		return frictionCoeff;
	}

	bool PhysicsComp::GetFlag(PHYSICS_COMP_FLAG flag) const
	{
		return flags.TestMask(flag);
	}

	void PhysicsComp::SetFlag(PHYSICS_COMP_FLAG flag, bool value)
	{
		flags.SetMask(flag, value);
	}

	bool PhysicsComp::IsDynamic() const
	{
		return GetFlag(PHYSICS_COMP_FLAG::IS_DYNAMIC);
	}
	bool PhysicsComp::IsGravityEnabled() const
	{
		return GetFlag(PHYSICS_COMP_FLAG::ENABLE_GRAVITY);
	}
	void PhysicsComp::SetIsDynamic(bool isDynamic)
	{
		return SetFlag(PHYSICS_COMP_FLAG::IS_DYNAMIC, isDynamic);
	}

	bool PhysicsComp::IsRotationLocked() const
	{
		return GetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED);
	}
	void PhysicsComp::SetIsRotationLocked(bool isRotationLocked)
	{
		SetFlag(PHYSICS_COMP_FLAG::ROTATION_LOCKED, isRotationLocked);
		if (isRotationLocked)
			angVelocity = 0.0f;
	}

#ifdef IMGUI_ENABLED
	void PhysicsComp::EditorDraw(PhysicsComp& comp)
	{
		comp.flags.MaskEditorDraw(physicsFlagNames);

		gui::VarDrag("Velocity", &comp.velocity, 10.0f, { -500.0f, -500.0f }, { 500.0f, 500.0f }, "%.1f");
		gui::VarDrag("Ang Velocity", &comp.angVelocity, 10.0f, -500.0f, 500.0f, "%.1f");
		gui::VarDrag("Mass", &comp.mass, 0.02f, 0.01f, 50.0f);
		gui::VarDrag("Restitution", &comp.restitutionCoeff, 0.002f, 0.0f, 1.0f);
		gui::VarDrag("Friction", &comp.frictionCoeff, 0.002f, 0.0f, 1.0f, "%.2f");
	}
#endif

	void PhysicsComp::Serialize(Serializer& writer) const
	{
		flags.MaskSerialize(writer, "flags", physicsFlagNames);

		writer.Serialize("mass", mass);
		writer.Serialize("restitutionCoeff", restitutionCoeff);
		writer.Serialize("frictionCoeff", frictionCoeff);
		writer.Serialize("angVelocity", angVelocity);
		writer.Serialize("velocity", velocity);
	}

	void PhysicsComp::Deserialize(Deserializer& reader)
	{
		flags.MaskDeserialize(reader, "flags", physicsFlagNames);

		reader.DeserializeVar("mass", &mass);
		reader.DeserializeVar("restitutionCoeff", &restitutionCoeff);
		reader.DeserializeVar("frictionCoeff", &frictionCoeff);
		reader.DeserializeVar("angVelocity", &angVelocity);
		reader.DeserializeVar("velocity", &velocity);
	}

#pragma endregion // PhysicsComp

#pragma region PhysicsSystem

	PhysicsSystem::PhysicsSystem()
		: SystemOperatingByLayer{ &PhysicsSystem::UpdatePhysComp }
		, gravity{ -980.0f }
		, dt{}
		, emptyComp{ PhysicsCompParams{.isDynamic = false } } // Make emptyComp static
		, simulationCenter{}
		, simulationHalfLength{ 2048.0f }
	{
	}

	void PhysicsSystem::OnAdded()
	{
		Messaging::Subscribe("OnCollision", PhysicsSystem::OnCollisionOccured);
	}

	void PhysicsSystem::OnRemoved()
	{
		Messaging::Unsubscribe("OnCollision", PhysicsSystem::OnCollisionOccured);
	}

	bool PhysicsSystem::PreRun()
	{
		dt = GameTime::FixedDt();
		// In case we're in variable dt mode, limit timestep
		if (!GameTime::IsFixedDtMode() && dt > 1.0f / 60.0f)
			dt = 1.0f / 60.0f;

		// Setup simulation area
		/*auto playerCompsIter{ ecs::GetCompsBegin<PlayerComponent>() };
		if (playerCompsIter != ecs::GetCompsEnd<PlayerComponent>())
			simulationCenter = playerCompsIter.GetEntity()->GetTransform().GetWorldPosition();
		simulationHalfLength = ST<GameSettings>::Get()->m_physicsSimulationSize * 0.5f;*/

		return true;
	}

	void PhysicsSystem::ProcessCollision(ColliderComp& refComp, ecs::CompHandle<PhysicsComp> refPhysComp, ColliderComp& otherComp, ecs::CompHandle<PhysicsComp> otherPhysComp, const CollisionData* collisionData)
	{
		// Assume at least one of the physics comps are valid due to how collision system checks for collisions.
		// If either physics comps are null, use emptyComp to simulate default physics parameters.
		if (!otherPhysComp)
			otherPhysComp = &emptyComp;
		else if (!refPhysComp)
			refPhysComp = &emptyComp;

		// References to shorten names
		const Vector2& normal{ collisionData->collisionNormal };
		const Vector2& point{ collisionData->collisionPoint };

		auto compIter{ ecs::GetCompsBegin<PointTestComp>() };
		if (compIter != ecs::GetCompsEnd<PointTestComp>())
			ecs::GetEntity(compIter.GetComp())->GetTransform().SetWorldPosition(point);

		Transform& refTransform{ ecs::GetEntityTransform(&refComp) };
		Transform& otherTransform{ ecs::GetEntityTransform(&otherComp) };

		// Get distance from center to collision point.
		Vector2 r1 = point - refTransform.GetWorldPosition();
		Vector2 r2 = point - otherTransform.GetWorldPosition();
		// Rotate these distances by 90 degrees to get perpendicular vector
		Vector2 r1Perp = r1.Rotate90();
		Vector2 r2Perp = r2.Rotate90();
		Vector2 angLinearVel1 = r1Perp * math::ToRadians(refPhysComp->GetAngVelocity());
		Vector2 angLinearVel2 = r2Perp * math::ToRadians(otherPhysComp->GetAngVelocity());
		// Calculate relative velocity between the 2 colliders at the collision point
		Vector2 relativeVelocity = (refPhysComp->GetVelocity() + angLinearVel1) - (otherPhysComp->GetVelocity() + angLinearVel2);

		// If colliders are heading away from the collision, we shouldn't need to do anything
		float dotVal = relativeVelocity.Dot(normal);
		if (dotVal >= 0.0f)
			return;

		// Get the component of the perpendicular distances in the direction of the collision normal
		float r1PerpDotNorm = r1Perp.Dot(normal);
		float r2PerpDotNorm = r2Perp.Dot(normal);

		// Calculate the restitution involved in this collision
		float avgRestitution = (refPhysComp->GetRestitutionCoeff() + otherPhysComp->GetRestitutionCoeff()) * 0.5f;
		// Calculate the velocity change value applied in the normal, ignoring mass/inertia for now
		float velChangeByNormal = -(relativeVelocity * (1.0f + avgRestitution)).Dot(normal);
		// Calculate the denominator consisting of mass and inertia that will make impulse take mass/inertia into account
		// but only include inertia if the physics collider is dynamic.
		float denominator = (refPhysComp->GetMassReciprocal() + otherPhysComp->GetMassReciprocal()); // mass
		if (refPhysComp->IsDynamic())
			denominator += math::PowSqr(r1PerpDotNorm) * refComp.GetMomentOfInertiaReciprocal(); // obj1 inertia
		if (otherPhysComp->IsDynamic())
			denominator += math::PowSqr(r2PerpDotNorm) * otherComp.GetMomentOfInertiaReciprocal(); // obj2 inertia
		// Get the impulse applied onto both objects
		float impulseMag = velChangeByNormal / denominator;
		Vector2 impulse = normal * impulseMag;

		// Calculate the friction involved in this collision
		float actingFriction = std::min(refPhysComp->GetFrictionCoeff(), otherPhysComp->GetFrictionCoeff());
		// Calculate the direction of friction
		Vector2 frictionDir = normal.Rotate90();
		if (relativeVelocity.Dot(frictionDir) > 0.0f)
			frictionDir = -frictionDir;
		// Calculate the velocity change value applied by friction, ignoring mass/inertia for now
		float velChangeByFriction = -(relativeVelocity * 0.5f * actingFriction).Dot(frictionDir);
		// Get the impulse applied by friction
		float fricImpulseMag = velChangeByFriction / denominator;
		impulse += frictionDir * fricImpulseMag * 100.0f * dt;

		// Apply linear and angular velocity changes
		refPhysComp->ApplyImpulse(impulse, r1, refComp.GetMomentOfInertia(), refComp.SupportsRotation());
		if (otherPhysComp->IsDynamic())
			otherPhysComp->ApplyImpulse(-impulse, r2, otherComp.GetMomentOfInertia(), otherComp.SupportsRotation());
		//LimitVelocity(obj1);
		//LimitVelocity(obj2);
	}

	void PhysicsSystem::UpdatePhysComp(PhysicsComp& physComp)
	{
		// TODO: Keep a list of dynamic components, then override Run() to implement our own iteration
		if (!physComp.IsDynamic())
			return;

		// Only update this object if it's within the simulation range
		Transform& transform{ ecs::GetEntityTransform(&physComp) };
		if (!QuadTree::CheckIsOverlapping(simulationCenter, simulationHalfLength, transform.GetWorldPosition(), transform.GetWorldScale() * 0.5f))
			return;

		// Apply gravity and update position
		Vector2 finalVelocity = physComp.GetVelocity();
		if (physComp.IsGravityEnabled())
			finalVelocity.y += gravity * dt;

		transform.AddWorldPosition((physComp.GetVelocity() + finalVelocity) * (0.5f * dt));
		physComp.SetVelocity(finalVelocity);

		if (!physComp.IsRotationLocked())
			transform.AddWorldRotation(physComp.GetAngVelocity() * dt);
	}

	void PhysicsSystem::OnCollisionOccured(const CollisionEventData& param)
	{
		// Don't apply velocity changes if this is a collision with a trigger.
		if (param.isCollisionWithTrigger)
			return;

		ecs::GetSystem<PhysicsSystem>()->ProcessCollision(*param.refComp, param.refPhysComp, *param.otherComp, param.otherPhysComp, param.collisionData);
	}

	PhysicsVelocityDebugSystem::PhysicsVelocityDebugSystem()
		: System_Internal{ &PhysicsVelocityDebugSystem::RenderComp }
	{
	}

	void PhysicsVelocityDebugSystem::RenderComp([[maybe_unused]] PhysicsComp& comp)
	{
		// We don't want to draw at all if IMGUI is disabled
#ifdef IMGUI_ENABLED
		if (!ST<Editor>::Get()->m_drawVelocity)
			return;

		// Don't draw a line for velocities too short to do so
		const Vector2& velocity{ comp.GetVelocity() };
		if (std::fabs(velocity.x) + std::fabs(velocity.y) <= std::numeric_limits<float>::epsilon())
			return;

		Vector2 pos{ ecs::GetEntity(&comp)->GetTransform().GetWorldPosition() };
		util::DrawLine(pos, pos + velocity, { 1.0f, 165.0f / 255.0f, 0 }, 1.0f);
#endif
	}

#pragma endregion PhysicsSystem

}
