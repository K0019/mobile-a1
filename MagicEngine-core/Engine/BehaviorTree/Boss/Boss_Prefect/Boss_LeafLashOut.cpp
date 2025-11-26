#include "Boss_LeafLashOut.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"

float L_Boss_Prefect_LashOut::attackCooldown = 3.0f;
float L_Boss_Prefect_LashOut::attackDistance = 2.0f * 2.0f;
float L_Boss_Prefect_LashOut::speedMultiplier = 3.0f;
int L_Boss_Prefect_LashOut::attackCount = 4;

void L_Boss_Prefect_LashOut::OnInitialize()
{
    currentAttackCooldown = attackCooldown;
    currentAttackCount = attackCount;
}

NODE_STATUS L_Boss_Prefect_LashOut::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        characterComp->SetSpeedMultiplier(speedMultiplier);
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());


            if (dir.LengthSqr() <= attackDistance)
            {
                if (currentAttackCooldown <= 0.0f)
                {
                    characterComp->Attack();
                    currentAttackCooldown = attackCooldown;

                    --currentAttackCount;
                    if (currentAttackCount == 0)
                    {
                        characterComp->ResetSpeedMultiplier();
                        return NODE_STATUS::SUCCESS;
                    }
                }
                    // Stop moving, it gives the player *some* time to get in a hit / ESCAPE
                    characterComp->SetMovementVector(Vec2{ 0.0f });
            }
            else
            {
                if (currentAttackCooldown <= 0.0f)
                {
                    characterComp->SetMovementVector(dir);
                }
            }

            currentAttackCooldown -= GameTime::Dt();
        }
    }
    return NODE_STATUS::RUNNING;
}