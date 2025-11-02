#include "LeafAttackPlayer.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Health.h"
#include "GameTime.h"
#include "Math/utils_math.h"

void L_AttackPlayer::OnInitialize()
{
    phase = AttackPhase::Idle;
    attackTimer = 0.0f;
    attackHit = false;
}

NODE_STATUS L_AttackPlayer::OnUpdate(ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp<EnemyComponent>();
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
        float desiredYaw = glm::degrees(atan2(toPlayer.x, -toPlayer.z));
        Vec3 newRot = entity->GetTransform().GetWorldRotation();
        newRot.y = desiredYaw;
        entity->GetTransform().SetWorldRotation(newRot);
    }

    // forward/right vectors
    Vec3 rotation = entity->GetTransform().GetWorldRotation();
    float yawRad = glm::radians(rotation.y);
    Vec3 forward{ sin(yawRad), 0.0f, -cos(yawRad) };
    Vec3 right{ forward.z, 0.0f, -forward.x };

    float forwardDist = Dot(toPlayer, forward);
    float sideDist = Dot(toPlayer, right);
    bool inAttackZone = (forwardDist > 0.0f && forwardDist <= attackRange &&
        std::abs(sideDist) <= attackWidth * 0.5f);

    //attack phases
    switch (phase)
    {
    case AttackPhase::Idle:
        if (inAttackZone)
        {
            phase = AttackPhase::Windup;
            attackTimer = 0.0f;
            attackHit = false;
        }
        break;

    case AttackPhase::Windup:
        attackTimer += GameTime::Dt();
        if (!inAttackZone)
        {
            // Player stepped out: cancel windup
            phase = AttackPhase::Idle;
            attackTimer = 0.0f;
            attackHit = false;
        }
        else if (attackTimer >= attackWindup)
        {
            phase = AttackPhase::Attack;
            attackTimer = 0.0f;
        }
        break;

    case AttackPhase::Attack:
        if (!attackHit)
        {
            attackHit = true;
            if (inAttackZone)
            {
                auto healthComp = player->GetComp<HealthComponent>();
                if (healthComp != nullptr)
                {
                    Vec3 knockDir = toPlayer.LengthSqr() > 0.0f ? toPlayer.Normalized() : forward;
                    healthComp->TakeDamage(attackDamage, knockDir);
                }
            }
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