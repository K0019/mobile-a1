/******************************************************************************/
/*!
\file   Physics.h
\par    Project: 7percent
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/09/2025

\author Takumi Shibamoto (100%)
\par    email: t.shibamoto\@digipen.edu
\par    DigiPen login: t.shibamoto

\brief


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "pch.h"
#include "Collision.h"
#include "Physics.h"
#include "NameComponent.h"

namespace physics {
	PhysicsComp::PhysicsComp()
		: bodyID {}
	{
	}

	void PhysicsComp::OnAttached()
	{
		if (ecs::GetEntity(this)->HasComp<BoxColliderComp>())
			bodyID = ecs::GetEntity(this)->GetComp<BoxColliderComp>()->GetBodyID();
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
	}

	JPH::BodyID PhysicsComp::GetBodyID()
	{
		return bodyID;
	}

	Vec3 const& PhysicsComp::GetBodyPosition()
	{
		JPH::RVec3Arg pos{ ST<JoltPhysics>::Get()->GetBodyInterface().GetPosition(bodyID) };
		return Vec3{ pos.GetX(), pos.GetY(), pos.GetZ() };
	}

	void PhysicsComp::EditorDraw()
	{

	}

	bool PhysicsSystem::PreRun()
	{
		ST<JoltPhysics>::Get()->UpdatePhysicsSystem();

		for (auto compIter{ ecs::GetCompsActiveBegin<PhysicsComp>() }, endIter{ ecs::GetCompsEnd<PhysicsComp>() }; compIter != endIter; ++compIter)
		{
			Vec3 pos{ compIter->GetBodyPosition() };
			compIter.GetEntity()->GetTransform().SetWorldPosition(pos);
		}

		return true;
	}
}