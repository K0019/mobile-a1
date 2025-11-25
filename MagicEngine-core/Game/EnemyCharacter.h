/******************************************************************************/
/*!
\file   EnemyCharacter.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	EnemyCharacter is an ECS component-system pair which controls enemy movement. 

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

/*****************************************************************//*!
\class EnemyComponent
\brief
	ECS Component that serializes relevant parameters.
*//******************************************************************/
class EnemyComponent
	: public IRegisteredComponent<EnemyComponent>
	, public IEditorComponent<EnemyComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	EnemyComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	*//******************************************************************/
	//~EnemyComponent();

	EntityReference playerReference;
	EntityReference attackCollider;
	float combatRange;
	float windUpTime;
	float attackTime;
	float attackCoolDownTime;
	float currAttackCoolDown;
	float followThroughTime;

	void Serialize(Serializer& writer) const override;
	void Deserialize(Deserializer& reader) override;

	property_vtable()

private:
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

};

property_begin(EnemyComponent)
{
	property_var(combatRange),
	property_var(windUpTime),
	property_var(attackTime),
	property_var(followThroughTime),
	property_var(attackCoolDownTime),
}
property_vend_h(EnemyComponent)

/*****************************************************************//*!
\class EnemyMovementComponentSystem
\brief
	ECS System that acts on the relevant component.
*//******************************************************************/
class EnemyMovementComponentSystem : public ecs::System<EnemyMovementComponentSystem, EnemyComponent>
{
public:
	EnemyMovementComponentSystem();
private:
	/*****************************************************************//*!
	\brief
		Updates EnemyComponent.
	\param comp
		The EnemyComponent to update.
	*//******************************************************************/
	void UpdateEnemyMovementComponent(EnemyComponent& comp);
};