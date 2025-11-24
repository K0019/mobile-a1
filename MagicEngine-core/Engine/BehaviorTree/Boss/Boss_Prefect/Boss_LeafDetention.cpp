#include "Boss_LeafDetention.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Engine/PrefabManager.h"
#include "Scripting/ScriptComponent.h"

const int L_Boss_Prefect_Detention::burstCount = 3;
const float L_Boss_Prefect_Detention::burstDelay = 0.5f;

const float L_Boss_Prefect_Detention::triggerDistance = 3.0f;

float L_Boss_Prefect_Detention::triggerDistanceSqr = L_Boss_Prefect_Detention::triggerDistance * L_Boss_Prefect_Detention::triggerDistance;


void L_Boss_Prefect_Detention::OnInitialize()
{
    explosionSizes.resize(L_Boss_Prefect_Detention::burstCount);

    // Init explosion sizes here
    explosionSizes[0] = 1.5f;
    explosionSizes[0] = 2.5f;
    explosionSizes[0] = 3.5f;

    currentBurstDelay = 0.0f;
    currentBurstCount = 0;
}

NODE_STATUS L_Boss_Prefect_Detention::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            // Directly follow player here, since this is an AoE around the boss
            Vec3 dir = enemyComp->playerReference->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition();
            dir.y = 0.0f;
            characterComp->SetMovementVector(Vec2{ dir.x,dir.z });
        
            if (dir.LengthSqr() < triggerDistanceSqr)
            {
                if (currentBurstDelay < 0.0f)
                {
                    ++currentBurstCount;
                    currentBurstDelay = burstDelay;

                    // TODO: DO BURST SPAWN HERE
                    ecs::EntityHandle spawnedExplosion = ST<PrefabManager>::Get()->LoadPrefab("EnemyExplosion");

                    if (auto scriptComp{ spawnedExplosion->GetComp<ScriptComponent>() })
                    {
                        scriptComp->CallScriptFunction("setSize", );
                    }

                    spawnedExplosion->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition());

                    if (currentBurstCount == burstCount)
                        return NODE_STATUS::SUCCESS;
                }
            }
        }
    }
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}