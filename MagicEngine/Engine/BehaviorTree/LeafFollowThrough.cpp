#include "Engine/BehaviorTree/LeafFollowThrough.h"
#include "Utilities/GameTime.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Graphics/AnimatorComponent.h"

void L_FollowThrough::OnInitialize()
{
	currSpentTime = 0.f;
}

NODE_STATUS L_FollowThrough::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	auto animComp{ entity->GetComp<AnimatorComponent>() };

	if (!enemyComp || !characterComp || !animComp)
		return NODE_STATUS::FAILURE;

	if (characterComp->currentStunTime > 0.f)
	{
		return NODE_STATUS::FAILURE;
	}

	return animComp->GetStateMachine()->GetBlackboardVal<bool>("attacking") ?
		NODE_STATUS::RUNNING :
		NODE_STATUS::SUCCESS;
}