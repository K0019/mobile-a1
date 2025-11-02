#include "LeafAttackPlayer.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Utilities/GameTime.h"
#include "Math/utils_math.h"
// Matthew here, I'm going to be performing the equivalent of sledgehammer surgery on this file, be prepared to rewrite all of it after M2

void L_AttackPlayer::OnInitialize()
{
    phase = AttackPhase::Idle;
    attackTimer = 0.0f;
    attackHit = false;
}

NODE_STATUS L_AttackPlayer::OnUpdate(ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp<EnemyComponent>();
    ecs::CompHandle<CharacterMovementComponent> characterComp = entity->GetComp<CharacterMovementComponent>();
    if (!enemyComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;
    if (!player)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    // calculate horizontal direction to player
    Vec3 toPlayer = playerPos - enemyPos;
    toPlayer.y = 0.0f;
    float distanceToPlayer = toPlayer.Length();

    // calculate rotation to face player
    if (distanceToPlayer > 0.0f)
    {
        characterComp->RotateTowards(Vec2{ toPlayer.x, -toPlayer.z });
        if (distanceToPlayer > 3.0f)
            return NODE_STATUS::FAILURE;
    }
    


    //attack phases
    switch (phase)
    {
    case AttackPhase::Idle:
            phase = AttackPhase::Windup;
        break;
    case AttackPhase::Windup:
        attackTimer += GameTime::Dt();

        if (attackTimer >= attackWindup)
        {
            phase = AttackPhase::Attack;
            attackTimer = 0.0f;
        }
        break;

    case AttackPhase::Attack:
        if (!attackHit)
        {
            attackHit = true;

            characterComp->Attack();
        }
        attackTimer += GameTime::Dt();
        if (attackTimer >= attackCooldown)
        {
            phase = AttackPhase::Idle;
            attackTimer = 0.0f;
        }
        break;
    }

    // node keeps running while attacking, completes after hit + cooldown
    return (phase == AttackPhase::Idle && attackHit) ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}