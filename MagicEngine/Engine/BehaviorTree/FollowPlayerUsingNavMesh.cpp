#include "FollowPlayerUsingNavMesh.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Graphics/AnimationComponent.h"


void L_FollowPlayerUsingNavMesh::OnInitialize()
{
    //reset pos;
    pathUpdateDuration = 0.25f;
    timer = pathUpdateDuration;
    currentCornerIndex = 0;
}

NODE_STATUS L_FollowPlayerUsingNavMesh::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    ecs::CompHandle<AnimationComponent> animComp = entity->GetComp<AnimationComponent>();
    ecs::CompHandle<CharacterMovementComponent> characterComp = entity->GetComp<CharacterMovementComponent>();
    ecs::CompHandle<navmesh::NavMeshAgentComp> agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    if (!enemyComp || !animComp || !characterComp || !agentComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    agentComp->SetActive(true);
    agentComp->SetTargetPos(playerPos);
    
    return NODE_STATUS::SUCCESS;
}