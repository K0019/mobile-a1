#include "Boss_LeafBookingSlips.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

int L_Boss_Prefect_BookingSlips::burstCount = 3;
float L_Boss_Prefect_BookingSlips::burstDelay = 0.25f;

void L_Boss_Prefect_BookingSlips::OnInitialize()
{
    //reset pos
    currentInvincinilityTime = invincibilityTime;
}

NODE_STATUS L_Boss_Prefect_BookingSlips::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        characterComp->SetMovementVector(Vec2{ 0.0f });
    }
    if (auto healthComp{ entity->GetComp<HealthComponent>() })
    {
        healthComp->SetIsInvincible(true);
        currentInvincinilityTime -= GameTime::Dt();

        if (currentInvincinilityTime <= 0.0f)
        {
            healthComp->SetIsInvincible(false);
            return NODE_STATUS::SUCCESS;
        }
    }
    return NODE_STATUS::RUNNING;
}