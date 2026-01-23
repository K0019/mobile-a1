#include "Engine/BehaviorTree/Melee/Melee_DecoIsInCombatRange.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

void D_Melee_IsInCombatRange::OnInitialize()
{
	runSpeed = 3.f;
}

NODE_STATUS D_Melee_IsInCombatRange::OnUpdate(ecs::EntityHandle entity)
{
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!enemyComp)
		return NODE_STATUS::FAILURE;

	EntityReference player{ enemyComp->playerReference };
	if (!player)
		return NODE_STATUS::FAILURE;

	//Check if the entity is inside the combat range.
	Vec3 posDiff{ player->GetTransform().GetWorldPosition() - entity->GetTransform().GetWorldPosition() };
	if (posDiff.LengthSqr() <= enemyComp->combatRange * enemyComp->combatRange)
		return NODE_STATUS::SUCCESS;

	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	if (!charComp)
		return NODE_STATUS::FAILURE;

	//Set the move speed to run.
	charComp->moveSpeed = runSpeed;
	NODE_STATUS status{ childPtr->Tick(entity) };
	return status == NODE_STATUS::SUCCESS ? NODE_STATUS::FAILURE : status;
}