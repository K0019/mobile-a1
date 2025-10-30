/******************************************************************************/
/*!
\file   Health.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\author Chan Kuan Fu Ryan (50%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
	Health component which stores a health type and broadcasts a message that it
	has died if the health goes below 0.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "ECS/EntityUID.h"
#include "Editor/IEditorComponent.h"

/*****************************************************************//*!
\class HealthComponent
\brief
	To attach to entities which need to keep track of health.
*//******************************************************************/
class HealthComponent 
	: public IRegisteredComponent<HealthComponent>
	, public  IEditorComponent<HealthComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	HealthComponent();

	/*****************************************************************//*!
	\brief
		Gets the current health.
	\return
		The current health.
	*//******************************************************************/
	int GetCurrHealth() const;

	/*****************************************************************//*!
	\brief
		Gets whether this entity is dead based on their current health.
	\return
		True if the entity is dead. False otherwise.
	*//******************************************************************/
	bool IsDead() const;

	/*****************************************************************//*!
	\brief
		Gets the max health.
	\return
		The max health.
	*//******************************************************************/
	int GetMaxHealth() const;

	/*****************************************************************//*!
	\brief
		Adds a certain amount of health.
	\param amount
		The amount of health to add.
	*//******************************************************************/
	void AddHealth(int amount);

	/*****************************************************************//*!
	\brief
		Subtracts a certain amount of health.
	\param amount
		The amount of damage to deal.
	*//******************************************************************/
	void TakeDamage(int amount);

	/*****************************************************************//*!
	\brief
		Sets the current health. Cannot exceed max health.
	\param newAmount
		The amount of health to set to.
	*//******************************************************************/
	void SetHealth(int newValue);

	/*****************************************************************//*!
	\brief
		Sets a new max health.
	\param newMaxAmount
		The new max health.
	*//******************************************************************/
	void SetMaxHealth(int newMaxAmount);

	float GetHealthFraction();
private:
	/*****************************************************************//*!
	\brief
		Draws this component to the inspector window.
	\param comp
		The health component to draw.
	*//******************************************************************/
	virtual void EditorDraw() override;

private:
	int maxHealth;
	int currHealth;

	static constexpr int defaultMax{ 100 };
	property_vtable()
};
property_begin(HealthComponent)
{
	property_var(maxHealth),
	property_var(currHealth),
}
property_vend_h(HealthComponent)