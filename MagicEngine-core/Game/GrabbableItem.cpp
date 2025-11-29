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

#include "Engine/PrefabManager.h"
#include "Graphics/RenderComponent.h"

void GrabbableItemComponent::Attack(Vec3 origin, Vec3 direction)
{
	//Add the damage if the attack is an ultimate attack.
	float attackDamage{ damage };
	if (auto playerComp{ ecs::GetEntity(this)->GetComp<PlayerMovementComponent>() })
	{
		if (playerComp->isUltimateAttack)
		{
			attackDamage += playerComp->ultimateAttackDamage;
			playerComp->isUltimateAttack = false;
		}
	}

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
			bool dealDamage = true;
			// If the target is parrying, we don't deal damage to them
			if(ecs::CompHandle<CharacterMovementComponent> characterComp{ hitEntity->GetComp<CharacterMovementComponent>() })
			{
				// Deal damage to it
				if (characterComp->IsParrying())
				{
					dealDamage = false;
					characterComp->OnParrySuccess();
				}
			}
			if(dealDamage)
				healthComp->TakeDamage(attackDamage, direction);
			//damage taken tied to delusion for now
			//if owner is enemy
			if (owner->GetComp<EnemyComponent>())
			{
				//hit player lose delusion
				if (ecs::CompHandle<DelusionComponent> delusionComp{ hitEntity->GetComp<DelusionComponent>() })
				{
					delusionComp->LoseDelusion(attackDamage * 0.2f);
				}
			}
			// if owner is player
			else 
			{
				//owner player gain delusion
				if (ecs::CompHandle<DelusionComponent> delusionComp{ owner->GetComp<DelusionComponent>() })
				{
					delusionComp->AddDelusion(attackDamage * 0.2f);
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

GrabbableItemPickupUISystem::GrabbableItemPickupUISystem()
	: System_Internal{ &GrabbableItemPickupUISystem::UpdateItemCompUI }
{
}

bool GrabbableItemPickupUISystem::PreRun()
{
	// If a UI entity we're tracking isn't valid anymore, highly likely that the scene has reset. If so, purge the UI entities.
	if (!(inactiveUIEntities.empty() || ecs::IsEntityHandleValid(inactiveUIEntities.front())) &&
		!(activeUIEntities.empty() || ecs::IsEntityHandleValid(activeUIEntities.begin()->second)))
	{
		inactiveUIEntities.clear();
		activeUIEntities.clear();
	}

	// If player doesn't exist, skip execution
	auto playerCompIter{ ecs::GetCompsActiveBeginConst<PlayerMovementComponent>() };
	if (playerCompIter == ecs::GetCompsEndConst<PlayerMovementComponent>())
		return false;

	// Save player position for distance calculation
	playerPos = playerCompIter.GetEntity()->GetTransform().GetWorldPosition();

	return true;
}

void GrabbableItemPickupUISystem::UpdateItemCompUI(GrabbableItemComponent& itemComp)
{
	constexpr float UI_APPEAR_DIST{ 2.5f };
	ecs::EntityHandle itemEntity{ ecs::GetEntity(&itemComp) };

	// Show/Hide UI based on distance check
	Vec3 toPlayer{ playerPos - itemEntity->GetTransform().GetWorldPosition()};
	if (toPlayer.LengthSqr() <= UI_APPEAR_DIST * UI_APPEAR_DIST)
		ShowUI(itemEntity, itemComp.pickupUI);
	else
		HideUI(itemEntity);
}

void GrabbableItemPickupUISystem::HideUI(ecs::EntityHandle itemEntity)
{
	// Ignore if this item entity doesn't have an active UI
	auto uiEntityIter{ activeUIEntities.find(itemEntity) };
	if (uiEntityIter == activeUIEntities.end())
		return;

	// Deactivate and unparent the UI
	ecs::EntityHandle uiEntity{ uiEntityIter->second };
	uiEntity->SetActive(false);
	
	// Transfer UI entity to inactive pool
	inactiveUIEntities.push_back(uiEntity);
	activeUIEntities.erase(uiEntityIter);
}

void GrabbableItemPickupUISystem::ShowUI(ecs::EntityHandle itemEntity, UserResourceHandle<ResourceMaterial> uiMaterial)
{
	Transform& itemTransform{ itemEntity->GetTransform() };

	// Assign a UI entity if this item doesn't have one yet
	auto uiEntityIter{ activeUIEntities.find(itemEntity) };
	if (uiEntityIter == activeUIEntities.end())
	{
		uiEntityIter = activeUIEntities.try_emplace(itemEntity, GetInactiveUIEntity()).first;
		uiEntityIter->second->SetActive(true);
		// Set texture (only works for single material mesh)
		if (auto renderComp{ uiEntityIter->second->GetComp<RenderComponent>() })
		{
			if (!renderComp->GetMaterialsList().empty())
				renderComp->GetMaterialsList()[0] = uiMaterial;
			else
				CONSOLE_LOG(LEVEL_ERROR) << "Item UI entity doesn't have a suitable mesh! Needs at least 1 material.";
		}
		else
			CONSOLE_LOG(LEVEL_ERROR) << "Item UI entity doesn't have a RenderComponent!";
	}

	// Set UI transform to just above the item
	Vec3 itemPos{ itemTransform.GetWorldPosition() };
	itemPos.y += 0.5f;
	uiEntityIter->second->GetTransform().SetWorldPosition(itemPos);
}

ecs::EntityHandle GrabbableItemPickupUISystem::GetInactiveUIEntity()
{
	if (!inactiveUIEntities.empty())
	{
		ecs::EntityHandle toReturn{ inactiveUIEntities.back() };
		inactiveUIEntities.pop_back();
		return toReturn;
	}
	
	// Safe as long as the prefab doesn't contain GrabbableItemComponent
	return ST<PrefabManager>::Get()->LoadPrefab("ItemPickupUI");
}
