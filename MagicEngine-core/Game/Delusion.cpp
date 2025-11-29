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
#include "Engine/EntityEvents.h"

#define X(enumName, str) str,
const std::array<std::string, +DELUSION_TIER::TOTAL> tierNames{
    DELUSION_TIER_ENUM
};

const std::string& DelusionComponent::TierToString(DELUSION_TIER tier)
{
    assert(tier != DELUSION_TIER::TOTAL);
    return tierNames[+tier];
}

DelusionComponent::DelusionComponent() :
	maxDelusion(defaultMax)
	, currDelusion{ 0.0f }
	, lossRate{ 1.0f }
	, gainRate{ 1.0f }
	, ultValue{ 90.0f }
	, prevTier{DELUSION_TIER::F}
	, currTier{DELUSION_TIER::F}
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
		(ratio >= 0.9f) ? DELUSION_TIER::APLUS :
		(ratio >= 0.8f) ? DELUSION_TIER::A :
		(ratio >= 0.6f) ? DELUSION_TIER::B :
		(ratio >= 0.4f) ? DELUSION_TIER::C :
		(ratio >= 0.2f) ? DELUSION_TIER::D :
		DELUSION_TIER::F;
    ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnDelusionChanged", ratio);

	if (currTier == prevTier) return;

	// Audio plays here
	ST<AudioManager>::Get()->PlaySound(currTier > prevTier ? "rank increase" : "rank decrease", false);

	//buffs and debuffs, as of yet unimplemented due to it being hard to see interactions, but ill throw in the pseudo code
    
    switch (currTier)
    {
    case DELUSION_TIER::F:
    {
        // Health and speed adjustments for DT_F
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(30);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
        }

        gainRate = 1.0f; // remove delusion gain increase
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DELUSION_TIER::D:
    {
        // Health and speed adjustments for DT_D
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(50);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.0f; // remove delusion gain increase
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DELUSION_TIER::C:
    {
        // Health and speed adjustments for DT_C
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(75);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f; // delusion gain up
        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DELUSION_TIER::B:
    {
        // Health and speed adjustments for DT_B
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(100);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f;

        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->AddHealth(20); // Health regen function not coded yet
        }

        lossRate = 1.0f; // remove delusion lose decrease
        break;
    }

    case DELUSION_TIER::A:
    {
        // Health and speed adjustments for DT_A
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(120);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
        {
            characterComp->ResetSpeedMultiplier();
            characterComp->SetSpeedMultiplier(1.1f); // speed buff
        }

        gainRate = 1.2f;
        lossRate = 0.8f; // delusion loss down
        break;
    }

    case DELUSION_TIER::APLUS:
    {
        // Health and speed adjustments for DT_Aplus
        if (ecs::CompHandle<HealthComponent> healthComp{ ecs::GetEntity(this)->GetComp<HealthComponent>() })
        {
            healthComp->SetMaxHealth(150);
        }

        if (ecs::CompHandle<CharacterMovementComponent> characterComp{ ecs::GetEntity(this)->GetComp<CharacterMovementComponent>() })
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
    
	prevTier = currTier;
    ecs::GetEntity(this)->GetComp<EntityEventsComponent>()->BroadcastAll("OnDelusionTierChanged", currTier);
}

DelusionComponent::DelusionType DelusionComponent::GetDelusionFraction()
{
	return (DelusionType)currDelusion / (DelusionType)maxDelusion;
}

void DelusionComponent::EditorDraw()
{
	gui::VarInput("Max Delusion", &maxDelusion); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Gain Rate", &gainRate); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Loss Rate", &lossRate); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Ult Value", &ultValue); //swap this out for one that prints to_print instead of &maxDelusion
	gui::VarInput("Current Delusion", &currDelusion); //swap this out for one that prints to_print instead of &maxDelusion
}
