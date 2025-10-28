#include "LeafMoveTowardsPlayer.h"
#include "BehaviourTreeFactory.h"

BT_REGISTER_NODE(L_MoveTowardsPlayer, "L_MoveTowardsPlayer")

void L_MoveTowardsPlayer::OnInitialize()
{
    //reset pos
}

NODE_STATUS L_MoveTowardsPlayer::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    ecs::EntityHandle player = nullptr;// ecs::FindEntityByName("Player");
    if (!player)
        return NODE_STATUS::FAILURE;

    Vec3 enemyPos = entity->GetTransform().GetWorldPosition();
    Vec3 playerPos = player->GetTransform().GetWorldPosition();

    Vec3 dir = playerPos - enemyPos;
    float dist = dir.Length();

    //stop if close to player
    if (dist < 1.0f)
        return NODE_STATUS::SUCCESS;

    //normalize
    float len = std::sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    if (len > 0.0f)
        dir = dir / len;

    // move towards player
    float dt = GameTime::Dt();
    entity->GetTransform().AddWorldPosition(dir * moveSpeed * dt);

    return NODE_STATUS::RUNNING;
}