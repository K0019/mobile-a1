#include "LeafMoveTowardsPlayer.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"


void L_MoveTowardsPlayer::OnInitialize()
{
    //reset pos
}

NODE_STATUS L_MoveTowardsPlayer::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    if (!enemyComp)
        return NODE_STATUS::FAILURE;

    ecs::CompHandle<CharacterMovementComponent> characterComp = entity->GetComp< CharacterMovementComponent>();
    if (!characterComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    Vec3 dir = playerPos - enemyPos;
    dir.y = 0.0f;

    //stop if close to player
    if (dir.Length() < 2.0f)
    {
        characterComp->SetMovementVector(Vec2(0.0f, 0.0f));
        return NODE_STATUS::SUCCESS;
    }


    // move towards player
    characterComp->SetMovementVector(Vec2(dir.x, -dir.z));
    return NODE_STATUS::RUNNING;
}