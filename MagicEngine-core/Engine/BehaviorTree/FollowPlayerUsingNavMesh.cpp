#include "FollowPlayerUsingNavMesh.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Engine/NavMeshAgent.h"


void L_FollowPlayerUsingNavMesh::OnInitialize()
{
    //reset pos
}

NODE_STATUS L_FollowPlayerUsingNavMesh::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    if (!enemyComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    auto agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    if (!agentComp)
        return NODE_STATUS::FAILURE;

    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    // move towards player
    agentComp->SetTargetPos(playerPos);
    return NODE_STATUS::SUCCESS;
}