#include "FollowPlayerUsingNavMesh.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Engine/NavMeshAgent.h"
#include "Game/Character.h"


void L_FollowPlayerUsingNavMesh::OnInitialize()
{
    //reset pos
}

NODE_STATUS L_FollowPlayerUsingNavMesh::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
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

    auto agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    if (!agentComp)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    // move towards player
    agentComp->SetTargetPos(playerPos);

    agentComp->SetActive(true);

    auto path{ agentComp->FindPath() };
    if (path.corners.size() > 0)
    {
        Vec3 dir = path.corners[0] - enemyPos;
        dir.y = 0.0f;

        // move towards player
        characterComp->SetMovementVector(Vec2(dir.x, dir.z));

    }
    
    return NODE_STATUS::SUCCESS;
}