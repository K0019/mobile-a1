#include "Boss_LeafDetention.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

int L_Boss_Prefect_Detention::burstCount = 3;
float L_Boss_Prefect_Detention::burstDelay = 0.5f;

float L_Boss_Prefect_Detention::triggerDistance = 3.0f;

float L_Boss_Prefect_Detention::triggerDistanceSqr = L_Boss_Prefect_Detention::triggerDistance * L_Boss_Prefect_Detention::triggerDistance;

void L_Boss_Prefect_Detention::OnInitialize()
{
    currentBurstDelay = 0.0f;
    currentBurstCount = 0;
}

NODE_STATUS L_Boss_Prefect_Detention::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec3 dir = enemyComp->playerReference->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition();
            dir.y = 0.0f;
            characterComp->SetMovementVector(Vec2{ dir.x,dir.z });
        
            if (dir.LengthSqr() < triggerDistanceSqr)
            {
                if (currentBurstDelay < 0.0f)
                {
                    ++currentBurstCount;

                    // TODO: DO BURST SPAWN HERE

                    if (currentBurstCount == burstCount)
                        return NODE_STATUS::SUCCESS;
                }
            }
        }
    }
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}