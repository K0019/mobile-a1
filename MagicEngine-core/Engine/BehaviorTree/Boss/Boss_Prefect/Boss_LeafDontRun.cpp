#include "Boss_LeafDontRun.h"
#include "Boss_Prefect_Util.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
float L_Boss_Prefect_DontRun::attackDistance = 2.0f*2.0f;
float L_Boss_Prefect_DontRun::dodgeDistance = 5.0f*5.0f;
int L_Boss_Prefect_DontRun::attackCount = 4;

void L_Boss_Prefect_DontRun::OnInitialize()
{
    currentAttackCount = attackCount;
}

NODE_STATUS L_Boss_Prefect_DontRun::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());

            if (dir.LengthSqr() > attackDistance)
            {
                characterComp->SetMovementVector(dir);
            }
            else
            {
                characterComp->RotateTowards(dir);
                if (characterComp->Attack())
                {
                    ++currentAttackCount;
                    if (currentAttackCount >= attackCount)
                        return NODE_STATUS::SUCCESS;
                }
            }

            if (dir.LengthSqr() > dodgeDistance)
            {
                characterComp->Dodge(dir);
            }
        }
    }
    return NODE_STATUS::RUNNING;
}