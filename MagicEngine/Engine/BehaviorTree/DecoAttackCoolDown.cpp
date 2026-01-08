#include "Engine/BehaviorTree/DecoAttackCoolDown.h"
#include "utilities/GameTime.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

NODE_STATUS D_AttackCoolDown::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!enemyComp || !characterComp || characterComp->currentStunTime > 0.f)
		return NODE_STATUS::FAILURE;

	if (enemyComp->currAttackCoolDown > enemyComp->attackCoolDownTime)
	{
		enemyComp->currAttackCoolDown = 0.f;
		return NODE_STATUS::FAILURE;
	}

	return childPtr->Tick(entity);
}