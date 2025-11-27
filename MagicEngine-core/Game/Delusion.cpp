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
#include "Game/Health.h"
#include "Character.h"
#include "PlayerCharacter.h"
#include "Physics//Physics.h"
#include "math/utils_math.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/SceneManagement.h"
#include "Managers/AudioManager.h"

std::string DelusionComponent::to_string()
{
	//map tier to alphabet
	static const std::map<DelusionTiers, std::string> names = {
		{DelusionTiers::DT_F, "F"}, {DelusionTiers::DT_D, "D"},
		{DelusionTiers::DT_C, "C"}, {DelusionTiers::DT_B, "B"},
		{DelusionTiers::DT_A, "A"}, {DelusionTiers::DT_APLUS, "A+"}
	};

	//returns alphabet name as std::string
	return names.at(currTier);
}

DelusionComponent::DelusionComponent() :
	maxDelusion(defaultMax)
	, currDelusion{ 0.0f }
	, lossRate{ 1.0f }
	, gainRate{ 1.0f }
	, ultValue{ 90.0f }
	, prevTier{DelusionTiers::DT_F}
	, currTier{DelusionTiers::DT_F}
    , owner {nullptr}
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
	currDelusion -= amount * lossRate;

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
	DelusionType ratio = GetDelusionFraction();
	currTier =
		(ratio >= 0.9f) ? DelusionTiers::DT_APLUS :
		(ratio >= 0.8f) ? DelusionTiers::DT_A :
		(ratio >= 0.6f) ? DelusionTiers::DT_B :
		(ratio >= 0.4f) ? DelusionTiers::DT_C :
		(ratio >= 0.2f) ? DelusionTiers::DT_D :
		DelusionTiers::DT_F;

	if (currTier == prevTier) return;

	// Audio plays here
	ST<AudioManager>::Get()->PlaySound(currTier > prevTier ? "rank increase" : "rank decrease", false);

	//buffs and debuffs, as of yet unimplemented due to it being hard to see interactions, but ill throw in the pseudo code
    /*
    switch (currTier)
    {
    case DelusionTiers::DT_F:
    {
        // Health and speed adjustments for DT_F
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(100);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
        }

        gainRate = 1.0f; // remove delusion gain increase
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DelusionTiers::DT_D:
    {
        // Health and speed adjustments for DT_D
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(80);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.0f; // remove delusion gain increase
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DelusionTiers::DT_C:
    {
        // Health and speed adjustments for DT_C
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(75);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f; // delusion gain up
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DelusionTiers::DT_B:
    {
        // Health and speed adjustments for DT_B
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(50);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f;

        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->AddHealth(20); // Health regen function not coded yet
        }

        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DelusionTiers::DT_A:
    {
        // Health and speed adjustments for DT_A
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(40);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f;
        lossRate = 0.8f; // delusion loss down
        break;
    }

    case DelusionTiers::DT_APLUS:
    {
        // Health and speed adjustments for DT_Aplus
        if (ecs::CompHandle<HealthComponent> healthComp{ owner->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(30);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ owner->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f;
        lossRate = 0.8f;
        
        // ulti available
        break;
    }

    default:
        // Handle unexpected case (optional)
        break;
    }
    */
	prevTier = currTier;
}

DelusionComponent::DelusionType DelusionComponent::GetDelusionFraction()
{
	return (DelusionType)currDelusion / (DelusionType)maxDelusion;
}

void DelusionComponent::EditorDraw()
{
	std::string to_print = to_string();
	gui::VarInput("Max Delusion", &maxDelusion); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Gain Rate", &gainRate); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Loss Rate", &lossRate); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Ult Value", &ultValue); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Current Delusion", &currDelusion); //swap this out for one that prints to_print instead of &maxDelusion
}
