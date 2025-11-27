#include "Engine/BehaviorTree/LeafRotateTowardsTarget.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"

NODE_STATUS L_RotateTowardsTarget::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!characterComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	auto playerRef{ enemyComp->playerReference };
	if (!playerRef)
		return NODE_STATUS::FAILURE;

	auto playerPos{ playerRef->GetTransform().GetWorldPosition() };
	auto enemyPos{ entity->GetTransform().GetWorldPosition() };

	//Rotate towards player
	characterComp->RotateTowards(Vec2(playerPos.x - enemyPos.x, playerPos.z - enemyPos.z));
	return NODE_STATUS::SUCCESS;
}