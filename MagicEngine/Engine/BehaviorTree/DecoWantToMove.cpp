#include "Engine/BehaviorTree/DecoWantToMove.h"
#include "Engine/NavMeshAgent.h"
#include "BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Graphics/AnimationComponent.h"

NODE_STATUS D_WantToMove::OnUpdate(ecs::EntityHandle entity)
{
    ecs::CompHandle<EnemyComponent> enemyComp = entity->GetComp< EnemyComponent>();
    auto agentComp{ entity->GetComp<navmesh::NavMeshAgentComp>() };
    auto animComp{ entity->GetComp<AnimationComponent>() };
    auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
    if (!enemyComp || !agentComp || !animComp || !characterComp || characterComp->currentStunTime > 0.f)
        return NODE_STATUS::FAILURE;

    ecs::EntityHandle player = enemyComp->playerReference;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    enemyComp->currAttackCoolDown += GameTime::Dt();

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    Vec3 displacement{ playerPos.x - enemyPos.x, 0.f, playerPos.z - enemyPos.z };
    if (displacement.LengthSqr() <= enemyComp->combatRange * enemyComp->combatRange)
    {
        characterComp->SetMovementVector(Vec2{ 0.f, 0.f });
        agentComp->SetActive(false);
        return NODE_STATUS::FAILURE;
    }

    agentComp->SetActive(true);
    NODE_STATUS stat{ childPtr->Tick(entity) };
    return stat;
}