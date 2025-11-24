#include "Boss_LeafBookingSlips.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
#include "Engine/PrefabManager.h"

int L_Boss_Prefect_BookingSlips::burstCount = 3;
float L_Boss_Prefect_BookingSlips::burstDelay = 0.25f;

void L_Boss_Prefect_BookingSlips::OnInitialize()
{
    currentBurstDelay = burstDelay;
}

NODE_STATUS L_Boss_Prefect_BookingSlips::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            // Use orbiting movement here
            characterComp->SetMovementVector(Boss_Prefect_Util::GetMovementDirection(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition()));

            if (currentBurstDelay < 0.0f)
            {
                ++currentBurstCount;
                currentBurstDelay = burstDelay;
            
                // TODO: DO BURST SPAWN HERE
                ecs::EntityHandle spawnedSlip = ST<PrefabManager>::Get()->LoadPrefab("prefect_bookingslip");

            
                if (currentBurstCount >= burstCount)
                    return NODE_STATUS::SUCCESS;
            }
            
        }
    }
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}