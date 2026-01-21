#include "Engine/BehaviorTree/Melee/Melee_DecoIsNotCurrentAttack.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"

void D_Melee_IsNotCurrentAttack::OnInitialize()
{
	runSpeed = 3.f;
	walkSpeed = 0.4f;
}

NODE_STATUS D_Melee_IsNotCurrentAttack::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	if (!charComp)
		return NODE_STATUS::FAILURE;

	//Check if current attacking is the entity.
	if (!BehaviorTree::globalBlackBoard.HasKey("MeleeAttacking"))
		return NODE_STATUS::FAILURE;
	else
	{
		ecs::EntityHandle handle{};
		BehaviorTree::globalBlackBoard.GetValue("MeleeAttacking", handle);
		if (handle == entity)
		{
			charComp->moveSpeed = runSpeed;
			NODE_STATUS status = childPtr->Tick(entity);
			return NODE_STATUS::FAILURE;
		}
	}

	charComp->moveSpeed = walkSpeed;
	return NODE_STATUS::SUCCESS;
}