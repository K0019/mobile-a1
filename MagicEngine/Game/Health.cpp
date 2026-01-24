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
#include "PlayerCharacter.h"
#include "FlashComponent.h"
#include "Physics//Physics.h"
#include "math/utils_math.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/SceneManagement.h"
#include "Scripting/ScriptComponent.h"
#include "Engine/EntityEvents.h"
#include "VFS/VFS.h"
#include "FilepathConstants.h"

HealthComponent::HealthType cheatState = 0;
bool cheatActive = false; ///THis is so the healthbar colour wont keep updating

void HealthComponent::OnStart()
{
	// Send this to force any attached objects to update
	ST<Scheduler>::Get()->Add([thisComp = this] {	ecs::GetEntity(thisComp)->GetComp<EntityEventsComponent>()->BroadcastAll("OnHealthChanged", thisComp->GetHealthFraction());
		});
}

HealthComponent::HealthComponent()
	: maxHealth(defaultMax)
	, currHealth(maxHealth)
{
}

HealthComponent::HealthType HealthComponent::GetCurrHealth() const
{
	return currHealth;
}

float HealthComponent::GetCurrHealthNormalized() const
{
	// Cast in case we change the health type
	return (float)currHealth/(float)maxHealth;
}

bool HealthComponent::IsDead() const
{
	return currHealth <= 0;
}

HealthComponent::HealthType HealthComponent::GetMaxHealth() const
{
	return maxHealth;
}


void HealthComponent::AddHealth(HealthType amount)
{
	currHealth += amount;
	if (currHealth > maxHealth)
		currHealth = maxHealth;

	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnHealthChanged", GetHealthFraction());

	if (auto scriptComp{ ecs::GetEntity(this)->GetComp<ScriptComponent>() })
		scriptComp->CallScriptFunction("OnHealed", amount);
}

void HealthComponent::TakeDamage(HealthComponent::HealthType amount, Vec3 direction)
{
	if (IsDead())
		return;

	// If the healthComp is already invincible, we can't do anything to the health
	if (isInvincible)
		return;

	auto thisEntity = ecs::GetEntity(this);

	if (currHealth > maxHealth)
		currHealth = maxHealth;
	currHealth -= amount;

	thisEntity->GetComp<EntityEventsComponent>()->BroadcastAll("OnHealthChanged", GetHealthFraction());

	// We don't need to flash if the entity is already dead,
	// or this health component is invulnerable.
	if (IsDead())
	{
		if (auto playerComp{ thisEntity->GetComp< PlayerMovementComponent >() })
		{
			ST<Scheduler>::Get()->Add([]() { ST<SceneManager>::Get()->UnloadAllScenes(VFS::JoinPath(Filepaths::scenesSave, "losemenu.scene")); });
		}
		else
		{
			if (auto scriptComp{ thisEntity->GetComp<ScriptComponent>() })
				scriptComp->CallScriptFunction("OnHealthDepleted");
			ecs::DeleteEntity(thisEntity);
		}
		return;
	}
	if (auto scriptComp{ thisEntity->GetComp<ScriptComponent>() })
		scriptComp->CallScriptFunction("OnDamaged", amount, direction);


	// Stun the character
	if (auto characterComp{ thisEntity->GetComp< CharacterMovementComponent >() })
	{
		characterComp->currentStunTime = characterComp->stunTimePerHit;
	}

	// Add the force
	if (direction.LengthSqr() > 0.0f)
	{
		if (auto physicsComp{ thisEntity->GetComp< physics::PhysicsComp >() })
		{
			// Disabled: Causes flying???
			Vec3 impulse = direction * amount;// +Vec3{ 0.0f,amount,0.0f };
			impulse = impulse *0.1f;
			physicsComp->SetLinearVelocity(impulse);
		}
	}

	// Trigger a flash
	if (auto flashComp{ thisEntity->GetComp< FlashComponent >() })
	{
		flashComp->Flash();
	}
}

void HealthComponent::SetHealth(HealthType newValue)
{
	if (newValue < 0)
		newValue = 0;
	if (newValue > maxHealth)
		newValue = maxHealth;

	// Set and update values
	currHealth = newValue;

	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnHealthChanged", GetHealthFraction());
}

void HealthComponent::SetMaxHealth(HealthType newMaxAmount)
{
	maxHealth = newMaxAmount;
	if (currHealth > maxHealth)
		currHealth = maxHealth;

	ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnHealthChanged", GetHealthFraction());
}


float HealthComponent::GetHealthFraction()
{
	return (float)currHealth/(float)maxHealth;
}

bool HealthComponent::GetIsInvincible() const
{
	return isInvincible;
}

void HealthComponent::SetIsInvincible(bool invincible)
{
	isInvincible = invincible;
}

void HealthComponent::EditorDraw()
{
	gui::VarInput("Curr Health", &currHealth);
	gui::VarInput("Max Health", &maxHealth);
	gui::VarDefault("Is Invincible", &isInvincible);
}
