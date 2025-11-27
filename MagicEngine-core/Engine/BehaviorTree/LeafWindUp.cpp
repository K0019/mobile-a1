#include "Engine/BehaviorTree/LeafWindUp.h"
#include "Utilities/GameTime.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "graphics/AnimationComponent.h"

void L_WindUp::OnInitialize()
{
	currSpentTime = 0.f;
}

NODE_STATUS L_WindUp::OnUpdate(ecs::EntityHandle entity)
{
	auto animComp{ entity->GetComp<AnimationComponent>() };
	auto characterComp{entity->GetComp<CharacterMovementComponent>()};
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!enemyComp || !characterComp)
		return NODE_STATUS::FAILURE;

	if (characterComp->currentStunTime > 0.f)
	{
		characterComp->isAttacking = false;
		return NODE_STATUS::FAILURE;
	}

	characterComp->isAttacking = true;
	animComp->animHandleA = characterComp->animations[ATTACK];
	currSpentTime += GameTime::Dt();

	if (currSpentTime < enemyComp->windUpTime)
		return NODE_STATUS::RUNNING;

	return NODE_STATUS::SUCCESS;
}