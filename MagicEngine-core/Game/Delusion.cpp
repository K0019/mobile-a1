/******************************************************************************/
/*!
\file   Delusion.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Matthew Chan Shao Jie (50%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\author Ng Jia Cheng (50%)
\par    email: c.n.jiacheng\@digipen.edu
\par    DigiPen login: c.n.jiacheng

\brief
	Delusion component which stores a Delusion type and broadcasts a message that it
	has died if the Delusion goes below 0.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Delusion.h"
#include "Character.h"
#include "PlayerCharacter.h"
#include "Physics//Physics.h"
#include "math/utils_math.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/SceneManagement.h"

std::string DelusionComponent::to_string()
{
	//map tier to alphabet
	static const std::map<DelusionTiers, std::string> names = {
		{DelusionTiers::DT_F, "F"}, {DelusionTiers::DT_D, "D"},
		{DelusionTiers::DT_C, "C"}, {DelusionTiers::DT_B, "B"},
		{DelusionTiers::DT_A, "A"}, {DelusionTiers::DT_APLUS, "A+"}
	};

	//returns alphabet name as std::string
	return names.at(currentTier);
}

DelusionComponent::DelusionComponent() :
	maxDelusion(defaultMax)
	, currDelusion(0)
{
}

DelusionComponent::DelusionType DelusionComponent::GetCurrDelusion() const
{
	return currDelusion;
}

bool DelusionComponent::IsUltReady() const
{
	return currDelusion >= ultValue;
}

DelusionComponent::DelusionType DelusionComponent::GetMaxDelusion() const
{
	return maxDelusion;
}

void DelusionComponent::AddDelusion(DelusionType amount)
{
	currDelusion += amount * gainRate;

	if (currDelusion > maxDelusion)
		currDelusion = maxDelusion;
	UpdateDelusionTier();
}

//should also be called after not doing anything for a while... or maybe a separate function like unity, idk
void DelusionComponent::LoseDelusion(DelusionType amount)
{
	currDelusion -= amount * loseRate;

	if (currDelusion < 0)
		currDelusion = 0;
	UpdateDelusionTier();
}

void DelusionComponent::SetDelusion(DelusionType newValue)
{
	if (newValue < 0)
		newValue = 0;
	if (newValue > maxDelusion)
		newValue = maxDelusion;

	// Set and update values
	currDelusion = newValue;
	UpdateDelusionTier();
}

void DelusionComponent::SetMaxDelusion(DelusionType newMaxAmount)
{
	maxDelusion = newMaxAmount;
	if (currDelusion > maxDelusion)
		currDelusion = maxDelusion;
	UpdateDelusionTier();
}

void DelusionComponent::UpdateDelusionTier()
{
	float ratio = GetDelusionFraction();
	currentTier =
		(ratio >= 0.9f) ? DelusionTiers::DT_APLUS :
		(ratio >= 0.8f) ? DelusionTiers::DT_A :
		(ratio >= 0.6f) ? DelusionTiers::DT_B :
		(ratio >= 0.4f) ? DelusionTiers::DT_C :
		(ratio >= 0.2f) ? DelusionTiers::DT_D :
		DelusionTiers::DT_F;

	if (currentTier == prevTier) return;

	//buffs and debuffs, as of yet unimplemented due to it being hard to see interactions, but ill throw in the pseudo code
	if (currentTier == DelusionTiers::DT_F)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(100);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() }), 
		// characterComp->moveSpeed = characterComp->baseMoveSpeed //comment: base ms doesn't exist but can store reference in delusion or add there
		// comment: remove speed buff
		// gainRate = 1.0f;
		// comment: remove delusion gain increase
		// turn off health regen if exists
		//loseRate = 1.0f;
		//comment: remove delusion lose decrease
	}
	else if (currentTier == DelusionTiers::DT_D)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(90);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() }), 
		// characterComp->moveSpeed = characterComp->baseMoveSpeed //comment: base ms doesn't exist but can store reference in delusion or add there
		// comment: speed buff, moveSpeed from CharacterMovementComponent
	}
	else if (currentTier == DelusionTiers::DT_C)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(80);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// gainRate = 1.2f;
	}
	else if (currentTier == DelusionTiers::DT_B)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(70);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->AddHealth(20);
		// comment: health regen function not coded, should be toggleable on and off and not a 1 time burst
	}
	else if (currentTier == DelusionTiers::DT_A)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(60);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// loseRate = 0.8f;
	}
	else if (currentTier == DelusionTiers::DT_APLUS)
	{
		// comment: probably store reference to original health max from health component on awake, then get health component 
		// if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() }), 
		// comment: delusion and health should both be on player after all
		// healthComp->SetMaxHealth(50);
		// comment: hundred being hardcoded value, but it should be taking from the max health reference stored from on awake
		// ulti available, no buff for now
	}
	prevTier = currentTier;
}

float DelusionComponent::GetDelusionFraction()
{
	return (float)currDelusion / (float)maxDelusion;
}

void DelusionComponent::EditorDraw()
{
	std::string to_print = to_string();
	gui::VarInput("Max Delusion", &maxDelusion); //swap this out for one that prints to_print instead of &maxDelusion
}
