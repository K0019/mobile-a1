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

    //timer += GameTime::Dt();
    //if (timer >= pathUpdateDuration)
    //{
    //    agentComp->SetTargetPos(playerPos);
    //    timer = 0.f;
    //}

    // move towards player
    timer += GameTime::Dt();
    if (timer >= pathUpdateDuration)
    {
        timer = 0.f;
        path = agentComp->FindPath(playerPos);
    }

    if (path.status == navmesh::NavMeshPathStatus::PATH_COMPLETE && path.corners.size() > 0)
    {
        // Move towards the current corner
        Vec3 currentTargetCorner = path.corners[std::min(currentCornerIndex + 1, path.corners.size() - 1)];
        //transform.position = Vector3.MoveTowards(transform.position, currentTargetCorner, moveSpeed * Time.deltaTime);
        Vec3 desiredDirection = currentTargetCorner - enemyPos;

        characterComp->SetMovementVector(Vec2(desiredDirection.x, desiredDirection.z));

        // Check if reached the current corner
        if (desiredDirection.Length() < 0.1f)
            currentCornerIndex++;
    }
    else
    {
        Vec3 desiredDirection = playerPos - enemyPos;
        characterComp->SetMovementVector(Vec2(desiredDirection.x, desiredDirection.z));
    }
    
    return NODE_STATUS::SUCCESS;
}