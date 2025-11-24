#include "Engine/BehaviorTree/DecoWantToMove.h"
#include "Engine/NavMeshAgent.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

NODE_STATUS D_WantToMove::OnUpdate(ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    if (!enemyComp)
        return NODE_STATUS::FAILURE;

    auto agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    if (!agentComp)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    Vec3 displacement{ playerPos.x - enemyPos.x, 0.f, playerPos.z - enemyPos.z };
    if (displacement.LengthSqr() <= enemyComp->combatRange * enemyComp->combatRange)
    {
        agentComp->SetActive(false);
        return NODE_STATUS::FAILURE;
    }

    agentComp->SetActive(true);
    NODE_STATUS stat{ childPtr->Tick(entity) };
    return stat;
}