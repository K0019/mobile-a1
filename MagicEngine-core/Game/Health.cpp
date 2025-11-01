/******************************************************************************/
/*!
\file   Health.cpp
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

#include "Health.h"
#include "Character.h"
#include "Physics//Physics.h"
#include "math/utils_math.h"
#include "Editor/Containers/GUICollection.h"

HealthComponent::health_type cheatState = 0;
bool cheatActive = false; ///THis is so the healthbar colour wont keep updating

HealthComponent::HealthComponent() :
	maxHealth(defaultMax)
	, currHealth(maxHealth)
{
}

HealthComponent::health_type HealthComponent::GetCurrHealth() const
{
	return currHealth;
}

bool HealthComponent::IsDead() const
{
	return currHealth <= 0;
}

HealthComponent::health_type HealthComponent::GetMaxHealth() const
{
	return maxHealth;
}


void HealthComponent::AddHealth(health_type amount)
{
	currHealth += amount;
	if (currHealth > maxHealth)
		currHealth = maxHealth;
}

void HealthComponent::TakeDamage(HealthComponent::health_type amount, Vec3 direction)
{
	currHealth -= amount;

	// We don't need to flash if the entity is already dead,
	// or this health component is invulnerable.
	if (IsDead())
	{
		ecs::DeleteEntity(ecs::GetEntity(this));
		return;
	}

	// Stun the character
	if (auto characterComp{ ecs::GetEntity(this)->GetComp< CharacterMovementComponent >() })
	{
		characterComp->currentStunTime = characterComp->stunTimePerHit;
	}

	// Add the force
	if (direction.LengthSqr() > 0.0f)
	{
		if (auto physicsComp{ ecs::GetEntity(this)->GetComp< physics::PhysicsComp >() })
		{
			auto hitVector = direction * amount;
			hitVector.y = 0;



			// Disabled: Causes flying???
			//physicsComp->SetLinearVelocity(direction * amount + Vec3{ 0.0f,amount,0.0f });
		}
	}
}

void HealthComponent::SetHealth(health_type newValue)
{
	if (newValue < 0)
		newValue = 0;
	if (newValue > maxHealth)
		newValue = maxHealth;

	// Set and update values
	currHealth = newValue;
}

void HealthComponent::SetMaxHealth(health_type newMaxAmount)
{
	maxHealth = newMaxAmount;
	if (currHealth > maxHealth)
		currHealth = maxHealth;
}


float HealthComponent::GetHealthFraction()
{
	return (float)currHealth/(float)maxHealth;
}

void HealthComponent::EditorDraw()
{
	ImGui::InputFloat("Max Health", &maxHealth);
}
