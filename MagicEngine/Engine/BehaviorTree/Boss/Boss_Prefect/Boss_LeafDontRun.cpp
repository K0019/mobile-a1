#include "Boss_LeafDontRun.h"
#include "Boss_Prefect_Util.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

float L_Boss_Prefect_DontRun::attackDistance = 20.0f*20.0f;
float L_Boss_Prefect_DontRun::attackCooldown = 3.0f;
float L_Boss_Prefect_DontRun::dodgeDistance = 25.0f*25.0f;
int L_Boss_Prefect_DontRun::attackCount = 4;

void L_Boss_Prefect_DontRun::OnInitialize()
{
    currentAttackCount = attackCount;
    currentAttackCooldown = attackCooldown;
}

NODE_STATUS L_Boss_Prefect_DontRun::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
   // if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    //{
        if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
        {
            Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());

            if (dir.LengthSqr() > attackDistance)
            {
                //characterComp->SetMovementVector(dir);
				Boss_Prefect_Util::MoveInDirection(entity, Vec3{ dir.x, 0.0f, dir.y });
                Boss_Prefect_Util::RotateTowards(entity, dir);
            }
            else
            {
				Boss_Prefect_Util::RotateTowards(entity, dir);
                
                auto animComp = entity->GetComp<AnimationComponent>();
                if (animComp)
                {
                    animComp->TransitionTo(5852846630766581163, 0.1f);
                    animComp->timeA = 0.0f;
                }
                if (currentAttackCooldown<=0.0f)
                {
                    ecs::EntityHandle spawnedSpawner = ST<PrefabManager>::Get()->LoadPrefab("prefect_dontrunspawner");
                    //ST<AudioManager>::Get()->PlaySound3D("boss ground strike", false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);

                    // Sanityyyyy
                    if (spawnedSpawner)
                    {
                        // Set the size in the LUA script
                        if (auto scriptComp{ spawnedSpawner->GetComp<ScriptComponent>() })
                        {
                            Vec3 tmpDir = enemyComp->playerReference->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition();
                            scriptComp->CallScriptFunction("setDirection", tmpDir);
                            //scriptComp->CallScriptFunction("setDirection", tmpDir.x, tmpDir.y, tmpDir.z);
                        }


                        spawnedSpawner->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition());
                        spawnedSpawner->GetTransform().SetWorldRotation(entity->GetTransform().GetWorldRotation());
                    }

                    currentAttackCooldown = attackCooldown;

                    ++currentAttackCount;
                    if (currentAttackCount >= attackCount)
                        return NODE_STATUS::SUCCESS;
                }
            }
            currentAttackCooldown -= GameTime::Dt();
        }
    //}
    return NODE_STATUS::RUNNING;
}