/******************************************************************************/
/*!
\file   PlayerCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	GrabbableItemComponent is an ECS component-system pair which controls player movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "ECS/IEditorComponent.h"
#include "Assets/AssetHandle.h"
#include "Assets/Types/AssetTypes.h"
#include "Game/Weapon.h"

/*****************************************************************//*!
\class GrabbableItemComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class GrabbableItemComponent
	: public IRegisteredComponent<GrabbableItemComponent>
	, public IEditorComponent<GrabbableItemComponent>
{
public:
	AssetHandle<WeaponInfo> weaponInfo;
	// TODO: Replace bottom with above weapon info handle...
	AssetHandle<ResourceAnimation> lightAttackAnimation;
	AssetHandle<ResourceAnimation> heavyAttackAnimation;
	AssetHandle<ResourceAnimation> ultimAttackAnimation;
	AssetHandle<ResourceAnimation> parryAnimation;

	AssetHandle<ResourceMaterial> pickupUI;


	void Attack(Vec3 origin, Vec3 extents);
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	GrabbableItemComponent();

	// Serialized
	float damage;
	Vec3 attackBox;
	float attackDelay;
	std::string audioName;
	int audioStartIndex;
	int audioEndIndex;

	// Not serialized
	bool isHeld;
	EntityReference owner;

	property_vtable()

		// ===== Lua helpers =====
public:
	float GetDamage() const { return damage; }
	void  SetDamage(float v) { damage = v; }

	bool  GetIsHeld() const { return isHeld; }
	void  SetIsHeld(bool v) { isHeld = v; }

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

};

property_begin(GrabbableItemComponent)
{
	property_var(weaponInfo),
	property_var(damage),
	property_var(attackBox),
	property_var(attackDelay),
	property_var(audioName),
	property_var(audioStartIndex),
	property_var(audioEndIndex),
	property_var(lightAttackAnimation),
	property_var(heavyAttackAnimation),
	property_var(ultimAttackAnimation),
	property_var(parryAnimation),
	property_var(pickupUI)
}
property_vend_h(GrabbableItemComponent)

/*****************************************************************//*!
\class GrabbableItemComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class GrabbableItemComponentSystem : public ecs::System<GrabbableItemComponentSystem, GrabbableItemComponent>
{
public:
	GrabbableItemComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates GrabbableItemComponent.
	\param comp
		The GrabbableItemComponent to update.
	*//******************************************************************/
	void UpdateGrabbableItemComponent(GrabbableItemComponent& comp);
};

class GrabbableItemPickupUISystem : public ecs::System<GrabbableItemPickupUISystem, GrabbableItemComponent>
{
public:
	GrabbableItemPickupUISystem();

	bool PreRun() override;
	void PostRun() override;

private:
	void UpdateItemCompUI(GrabbableItemComponent& itemComp);

	void HideUI(ecs::EntityHandle itemEntity);
	void ShowUI(ecs::EntityHandle itemEntity, AssetHandle<ResourceMaterial> uiMaterial);
	ecs::EntityHandle GetInactiveUIEntity();

private:
	Vec3 playerPos;

	std::unordered_map<ecs::EntityHandle, ecs::EntityHandle> activeUIEntities;
	std::vector<ecs::EntityHandle> inactiveUIEntities;
};
