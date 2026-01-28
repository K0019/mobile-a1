#include "Boss_LeafDetention.h"
#include "Boss_Prefect_Util.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Managers/AudioManager.h"

const int L_Boss_Prefect_Detention::burstCount = 3;
const float L_Boss_Prefect_Detention::burstDelay = 3.0f;

const float L_Boss_Prefect_Detention::triggerDistance = 3.0f;

float L_Boss_Prefect_Detention::triggerDistanceSqr = L_Boss_Prefect_Detention::triggerDistance * L_Boss_Prefect_Detention::triggerDistance;


void L_Boss_Prefect_Detention::OnInitialize()
{
    explosionSizes.resize(L_Boss_Prefect_Detention::burstCount);

    // Init explosion sizes here
    explosionSizes[0] = 2.5f;
    explosionSizes[1] = 4.5f;
    explosionSizes[2] = 6.5f;

    currentBurstDelay = 0.0f;
    currentBurstCount = 0;
}

NODE_STATUS L_Boss_Prefect_Detention::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    // This only functions as a get out of my bubble attack, we don't move and just explode on the spot
    if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
    {
        Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
        // Boss_Prefect_Util::MoveInDirection(entity, Vec3(dir.x,0.0f,dir.y)); // We do not want to move during this attack
        Boss_Prefect_Util::RotateTowards(entity, dir);
        
        if (dir.LengthSqr() < triggerDistanceSqr)
        {
            if (currentBurstDelay < 0.0f)
            {
                ST<PrefabManager>::Get()->LoadPrefab("explosion");
                //ST<AudioManager>::Get()->PlaySound3D("defence stance", false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);

                if(Boss_Prefect_Util::SpawnExplosion(entity, explosionSizes[currentBurstCount]))
                {
                    // Reset burst delay and update count
                    currentBurstDelay = burstDelay;
                    ++currentBurstCount;
                }

                if (currentBurstCount == burstCount)
                    return NODE_STATUS::SUCCESS;
            }
        }
    }
    
    currentBurstDelay -= GameTime::Dt();
    return NODE_STATUS::RUNNING;
}