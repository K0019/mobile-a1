/******************************************************************************/
/*!
\file   GameCameraController.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	GameCameraController is an ECS component-system pair which takes control of
	the camera when the default scene is loaded (game scene). It in in charge of
	making camera follow the player, map bounds, and any other such effects to be
	implemented now or in the future.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/GrabbableItem.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"
#include "Game/PlayerCharacter.h"
#include "Game/EnemyCharacter.h"

void GrabbableItemComponent::Attack(Vec3 origin, Vec3 direction)
{
	std::vector<physics::BoxColliderComp*> colliders;
	physics::OverlapBox(colliders, origin, Vec3(2,1,2), direction);

	for (auto collider : colliders)
	{
		ecs::EntityHandle hitEntity = ecs::GetEntity(collider);

		// Can't hit self or owner
		if (hitEntity == owner || hitEntity == ecs::GetEntity(this))
			continue;

		// Enemies can't hit each other
		if (owner->GetComp<EnemyComponent>() && hitEntity->GetComp<EnemyComponent>())
			continue;

		if (ecs::CompHandle<HealthComponent> healthComp{ hitEntity->GetComp<HealthComponent>() })
		{
			// Deal damage to it
			healthComp->TakeDamage(damage,direction);
		}
	}
}

GrabbableItemComponent::GrabbableItemComponent() :
	damage{ 1.0f },
	isHeld{ false },
	owner(nullptr)
{
}

void GrabbableItemComponent::Serialize(Serializer& writer) const
{
	writer.Serialize("damage",damage);
}

void GrabbableItemComponent::Deserialize(Deserializer& reader)
{
	reader.DeserializeVar("damage", &damage);
}

void GrabbableItemComponent::EditorDraw()
{
	gui::VarInput("Damage", &damage);
}

GrabbableItemComponentSystem::GrabbableItemComponentSystem()
	: System_Internal{ &GrabbableItemComponentSystem::UpdateGrabbableItemComponent }

{
}

void GrabbableItemComponentSystem::UpdateGrabbableItemComponent(GrabbableItemComponent& comp)
{
	ecs::EntityHandle itemEntity = ecs::GetEntity(&comp);
	ecs::CompHandle<physics::PhysicsComp> physicsComp = itemEntity->GetComp<physics::PhysicsComp>();
	ecs::CompHandle<physics::BoxColliderComp> colliderComp = itemEntity->GetComp<physics::BoxColliderComp>();

	physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, !comp.isHeld);
	physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::USE_GRAVITY, !comp.isHeld);
	colliderComp->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, !comp.isHeld);

	if (comp.isHeld)
	{
		physicsComp->SetAngularVelocity(Vec3{ 0 });
		physicsComp->SetLinearVelocity(Vec3{ 0 });
	}
}
