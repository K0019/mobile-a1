#include "Boss_LeafDontRun.h"
#include "Boss_Prefect_Util.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

void L_Boss_Prefect_DontRun::OnInitialize()
{
}

NODE_STATUS L_Boss_Prefect_DontRun::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
            // Don't move here
            characterComp->SetMovementVector(dir);
            characterComp->Dodge(dir);
        }
    }
    return NODE_STATUS::RUNNING;
}