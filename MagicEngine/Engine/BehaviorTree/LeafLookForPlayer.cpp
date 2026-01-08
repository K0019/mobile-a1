#include "LeafLookForPlayer.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"


NODE_STATUS L_LookForPlayer::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::CompHandle< EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    if (!enemyComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    //Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    //Vec3 playerPos = player->GetTransform().GetWorldPosition();

    //float distance = (playerPos - enemyPos).Length();
    //const float detectionRange = 10.0f; //tentative

    return NODE_STATUS::SUCCESS;
    //return (distance < detectionRange) ? NODE_STATUS::SUCCESS : NODE_STATUS::FAILURE;
}