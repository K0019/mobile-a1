#include "Boss_LeafDisciplinaryAction.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
int L_Boss_Prefect_DisciplinaryAction::lungeCount = 5;
float L_Boss_Prefect_DisciplinaryAction::explosionSize = 5.0f;

void L_Boss_Prefect_DisciplinaryAction::OnInitialize()
{
    hasExploded = true;
    currentLungeCount = lungeCount;
}

NODE_STATUS L_Boss_Prefect_DisciplinaryAction::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
            if (!characterComp->IsDodging())
            {
                // Send out an explosion if we aren't dodging and *haven't* done so alr
                if (!hasExploded)
                {
                    if(Boss_Prefect_Util::SpawnExplosion(entity,explosionSize))
                    {
                        hasExploded = true;
                    }
                }

                // Keep trying to dodge
                if (characterComp->Dodge(dir))
                {
                    // Once dodge succeeded, we get ready to explode again
                    hasExploded = false;
                }
            }
        }
    }
    return NODE_STATUS::RUNNING;
}