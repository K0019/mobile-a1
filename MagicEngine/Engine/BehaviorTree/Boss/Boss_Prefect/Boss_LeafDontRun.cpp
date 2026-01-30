#include "Boss_LeafDontRun.h"
#include "Boss_Prefect_Util.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

float L_Boss_Prefect_DontRun::attackDistance = 20.0f*20.0f;
float L_Boss_Prefect_DontRun::followThroughTime = 1.5f;  // Delay after attack completes

void L_Boss_Prefect_DontRun::OnInitialize()
{
    currentFollowThroughTime = 0.0f;
    hasStartedAttack = false;
    hasSpawnedProjectile = false;
    isFollowingThrough = false;
}

NODE_STATUS L_Boss_Prefect_DontRun::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
    {
        Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());

        // Follow through delay after attack is done
        if (isFollowingThrough)
        {
            currentFollowThroughTime -= GameTime::Dt();
            if (currentFollowThroughTime <= 0.0f)
            {
                return NODE_STATUS::SUCCESS;
            }
            return NODE_STATUS::RUNNING;
        }

        // If attack hasn't started yet, check distance condition
        if (!hasStartedAttack)
        {
            if (dir.LengthSqr() > attackDistance)
            {
                // Too far - move towards player
                Boss_Prefect_Util::MoveInDirection(entity, Vec3{ dir.x, 0.0f, dir.y });
                Boss_Prefect_Util::RotateTowards(entity, dir);
                return NODE_STATUS::RUNNING;
            }
            else
            {
                // In range - start the attack
                hasStartedAttack = true;
                
                // Play the jump/slam animation once
                auto animComp = entity->GetComp<AnimationComponent>();
                if (animComp)
                {
                    animComp->TransitionTo(5852846630766581163, 0.1f);
                    animComp->timeA = 0.0f;
                }
            }
        }

        // Attack has started - rotate towards player but don't move
        Boss_Prefect_Util::RotateTowards(entity, dir);

        // Spawn projectile once
        if (!hasSpawnedProjectile)
        {
            ecs::EntityHandle spawnedSpawner = ST<PrefabManager>::Get()->LoadPrefab("prefect_dontrunspawner");
            //ST<AudioManager>::Get()->PlaySound3D("boss ground strike", false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);

            if (spawnedSpawner)
            {
                // Set the direction in the LUA script
                if (auto scriptComp{ spawnedSpawner->GetComp<ScriptComponent>() })
                {
                    Vec3 tmpDir = enemyComp->playerReference->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition();
                    scriptComp->CallScriptFunction("setDirection", tmpDir);
                }

                spawnedSpawner->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition());
                spawnedSpawner->GetTransform().SetWorldRotation(entity->GetTransform().GetWorldRotation());
            }

            hasSpawnedProjectile = true;

            // Enter follow through state
            isFollowingThrough = true;
            currentFollowThroughTime = followThroughTime;
        }
    }
    return NODE_STATUS::RUNNING;
}