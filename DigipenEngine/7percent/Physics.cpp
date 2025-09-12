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
		: bodyID{}
	{
	}

	void PhysicsComp::OnAttached()
	{
		// Logging to console is like std::cout syntax.
		// You can get the entity this component is attached to via ecs::GetEntity() and passing in the component's address.
		CONSOLE_LOG(LEVEL_INFO) << "Physics component attached to entity " << ecs::GetEntity(this)
			// There are some components that are guaranteed to exist on all entities (see class EntitySpawnEvents)
			<< " of name " << ecs::GetEntity(this)->GetComp<NameComponent>()->GetName();

		bodyID = ST<JoltPhysics>::Get()->CreateAndAddEmptyBody(ecs::GetEntityTransform(this), JPH::EMotionType::Dynamic, +Layers::MOVING);
		if (bodyID.IsInvalid())
			CONSOLE_LOG(LEVEL_ERROR) << "Invalid Body ID generated while creating the Physics component";
	}

	void PhysicsComp::OnDetached()
	{
		// Be careful in OnDetached(), guaranteed-to-exist components may not exist anymore if the entity is being deleted outright
		ecs::CompHandle<NameComponent> nameComp{ ecs::GetEntity(this)->GetComp<NameComponent>() };
		CONSOLE_LOG(LEVEL_INFO) << "Physics component detached(removed) from entity " << ecs::GetEntity(this)
			<< " of name " << (nameComp ? nameComp->GetName() : "[unknown]");

		ST<JoltPhysics>::Get()->RemoveAndDestroyBody(bodyID);
	}

	JPH::BodyID PhysicsComp::GetBodyID()
	{
		return bodyID;
	}

	void PhysicsComp::EditorDraw()
	{

	}

	bool PhysicsSystem::PreRun()
	{
		ST<JoltPhysics>::Get()->UpdatePhysicsSystem();

		for (auto compIter{ ecs::GetCompsActiveBegin<PhysicsComp>() }, endIter{ ecs::GetCompsEnd<PhysicsComp>() }; compIter != endIter; ++compIter)
		{
			Vec3 pos{ ST<JoltPhysics>::Get()->GetBodyPosition(compIter->GetBodyID()) };
			compIter.GetEntity()->GetTransform().SetWorldPosition(pos);
		}

		return true;
	}
}