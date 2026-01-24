#include "Engine/BehaviorTree/LeafFollowThrough.h"
#include "Utilities/GameTime.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

void L_FollowThrough::OnInitialize()
{
	currSpentTime = 0.f;
}

NODE_STATUS L_FollowThrough::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!enemyComp || !characterComp)
		return NODE_STATUS::FAILURE;

	if (characterComp->currentStunTime > 0.f)
	{
		return NODE_STATUS::FAILURE;
	}

	currSpentTime += GameTime::Dt();

	if (currSpentTime < enemyComp->followThroughTime)
		return NODE_STATUS::RUNNING;

	return NODE_STATUS::SUCCESS;
}