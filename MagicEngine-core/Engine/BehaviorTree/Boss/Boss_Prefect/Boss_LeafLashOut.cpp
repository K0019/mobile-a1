#include "Boss_LeafLashOut.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Boss_Prefect_Util.h"
#include "Graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

float L_Boss_Prefect_LashOut::attackCooldown = 3.0f;
float L_Boss_Prefect_LashOut::attackDelay = 0.5f;
float L_Boss_Prefect_LashOut::attackDistance = 2.0f * 2.0f;
float L_Boss_Prefect_LashOut::speedMultiplier = 3.0f;
int L_Boss_Prefect_LashOut::attackCount = 4;

void L_Boss_Prefect_LashOut::OnInitialize()
{
    currentAttackCooldown = attackCooldown;
    currentAttackCount = attackCount;
    currentAttackDelay = attackDelay;
    //ST<AudioManager>::Get()->PlaySound3D("boss lunge", false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);
}

NODE_STATUS L_Boss_Prefect_LashOut::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    //if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    //{
        //characterComp->SetSpeedMultiplier(speedMultiplier);
    if (auto enemyComp{ entity->GetComp<EnemyComponent>() })
    {
        Vec2 dir = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());

        if (dir.LengthSqr() <= attackDistance || impendingAttack)
        {
            if (currentAttackDelay <= 0.0f)
            {
                auto animComp = entity->GetComp<AnimationComponent>();
                if (animComp)
                {
                    entity->GetComp<GrabbableItemComponent>()->Attack(entity->GetTransform().GetWorldPosition(), Vec3{ dir.x, 0.0f, dir.y });
                    animComp->TransitionTo(5858584981951944119, 0.1f);
                    animComp->timeA = 0.0f;
                }
                currentAttackDelay = attackDelay;

                impendingAttack = false;

                --currentAttackCount;
                if (currentAttackCount == 0)
                {
                    //characterComp->ResetSpeedMultiplier();
                    return NODE_STATUS::SUCCESS;
                }
                else
                {
                    //ST<AudioManager>::Get()->PlaySound3D("boss lunge", false, entity->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);
                }
            }
            currentAttackDelay -= GameTime::Dt();
            currentAttackCooldown = attackCooldown;

            // Stop moving, it gives the player *some* time to get in a hit / ESCAPE
            //characterComp->SetMovementVector(Vec2{ 0.0f });
        }
        else
        {
            currentAttackDelay = attackDelay;
            Boss_Prefect_Util::MoveInDirection(entity, Vec3{ dir.x, 0.0f, dir.y }*3);
            Boss_Prefect_Util::RotateTowards(entity, dir);
        }

        currentAttackCooldown -= GameTime::Dt();
    }
    //}
    return NODE_STATUS::RUNNING;
}