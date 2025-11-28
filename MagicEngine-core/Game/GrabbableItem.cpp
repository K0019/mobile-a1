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
#include "Game/Delusion.h"
#include "Managers\AudioManager.h"

void GrabbableItemComponent::Attack(Vec3 origin, Vec3 direction)
{
	//Add the damage if the attack is an ultimate attack.
	if (auto playerComp{ ecs::GetEntity(this)->GetComp<PlayerMovementComponent>() })
		if (playerComp->isUltimateAttack)
			damage += playerComp->ultimateAttackDamage;

	std::vector<physics::BoxColliderComp*> colliders;
	physics::OverlapBox(colliders, origin, attackBox, direction);

	for (auto collider : colliders)
	{
		ecs::EntityHandle hitEntity = ecs::GetEntity(collider);

		// Can't hit self or owner
		if (hitEntity == owner || hitEntity == ecs::GetEntity(this))
			continue;

		// If there's no owner, set the owner to itself
		if (!owner)
			owner = ecs::GetEntity(this);

		// Enemies can't hit each other
		if (owner->GetComp<EnemyComponent>() && hitEntity->GetComp<EnemyComponent>())
			continue;

		if (ecs::CompHandle<HealthComponent> healthComp{ hitEntity->GetComp<HealthComponent>() })
		{
			// Deal damage to it
			healthComp->TakeDamage(damage,direction);

			//damage taken tied to delusion for now
			//if owner is enemy
			if (owner->GetComp<EnemyComponent>())
			{
				//hit player lose delusion
				if (ecs::CompHandle<DelusionComponent> delusionComp{ hitEntity->GetComp<DelusionComponent>() })
				{
					delusionComp->LoseDelusion(damage * 0.2f);
				}
			}
			// if owner is player
			else 
			{
				//owner player gain delusion
				if (ecs::CompHandle<DelusionComponent> delusionComp{ owner->GetComp<DelusionComponent>() })
				{
					delusionComp->AddDelusion(damage * 0.2f);
				}
			}
		}
	}
}

GrabbableItemComponent::GrabbableItemComponent() :
	damage{ 1.0f },
	isHeld{ false },
	owner(nullptr),
	attackBox{},
	attackDelay{ 0.0f }
{
}

void GrabbableItemComponent::Serialize(Serializer& writer) const
{
	this->IRegisteredComponent::Serialize(writer);

	writer.Serialize(lightAttackAnimation);
	writer.Serialize(heavyAttackAnimation);
	writer.Serialize(ultimAttackAnimation);
	writer.Serialize(parryAnimation);
}

void GrabbableItemComponent::Deserialize(Deserializer& reader)
{
	this->IRegisteredComponent::Deserialize(reader);

	reader.Deserialize(&lightAttackAnimation);
	reader.Deserialize(&heavyAttackAnimation);
	reader.Deserialize(&ultimAttackAnimation);
	reader.Deserialize(&parryAnimation);
}

void GrabbableItemComponent::EditorDraw()
{
	gui::VarInput("Damage", &damage);
	gui::VarInput("Attack Box", &attackBox);
	gui::VarInput("Attack Delay", &attackDelay);

	// Animations
	gui::TextUnformatted("Light Attack");
	gui::SameLine();
	std::string blankName = "";
	gui::TextBoxReadOnly("##AnimClipLight", ST<MagicResourceManager>::Get()->Editor_GetName(lightAttackAnimation.GetHash()) ? (*ST<MagicResourceManager>::Get()->Editor_GetName(lightAttackAnimation.GetHash())) : blankName);
	gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
		lightAttackAnimation = hash;
		});

	gui::TextUnformatted("Heavy Attack");
	gui::SameLine();
	gui::TextBoxReadOnly("##AnimClipHeavy", ST<MagicResourceManager>::Get()->Editor_GetName(heavyAttackAnimation.GetHash()) ? (*ST<MagicResourceManager>::Get()->Editor_GetName(heavyAttackAnimation.GetHash())) : blankName);
	gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
		heavyAttackAnimation = hash;
		});

	gui::TextUnformatted("Ultimate");
	gui::SameLine();
	gui::TextBoxReadOnly("##AnimClipUltim", ST<MagicResourceManager>::Get()->Editor_GetName(ultimAttackAnimation.GetHash()) ? (*ST<MagicResourceManager>::Get()->Editor_GetName(ultimAttackAnimation.GetHash())) : blankName);
	gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
		ultimAttackAnimation = hash;
		});

	gui::TextUnformatted("Parry");
	gui::SameLine();
	gui::TextBoxReadOnly("##AnimClipParry", ST<MagicResourceManager>::Get()->Editor_GetName(parryAnimation.GetHash()) ? (*ST<MagicResourceManager>::Get()->Editor_GetName(parryAnimation.GetHash())) : blankName);
	gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
		parryAnimation = hash;
		});

	// TODO: Editor Draw
	gui::VarDefault("Audio Name", &audioName);

	gui::VarInput("Audio Start Index", &audioStartIndex);
	gui::VarInput("Audio End Index", &audioEndIndex);
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

	//physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, !comp.isHeld);
	//physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::USE_GRAVITY, !comp.isHeld);
	//physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::IS_KINEMATIC, comp.isHeld);
	//colliderComp->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, !comp.isHeld);

	if (comp.isHeld)
	{
		physicsComp->SetAngularVelocity(Vec3{ 0 });
		physicsComp->SetLinearVelocity(Vec3{ 0 });
	}
}
