#include "Melee_LeafCheckAttack.h"
#include "Game/Character.h"

NODE_STATUS L_Melee_CheckAttack::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };

	if (!charComp)
		return NODE_STATUS::FAILURE;

	//Check stun.
	if (charComp->currentStunTime > 0.f)
		return NODE_STATUS::FAILURE;

	//If there's no enemy attacking the player, set the entity to attacking.
	if (!BehaviorTree::globalBlackBoard.HasKey("MeleeAttacking"))
		BehaviorTree::globalBlackBoard.SetValue("MeleeAttacking", entity);
	else
	{
		ecs::EntityHandle handle{};
		BehaviorTree::globalBlackBoard.GetValue("MeleeAttacking", handle);
		if (!ecs::IsEntityHandleValid(handle))
			BehaviorTree::globalBlackBoard.SetValue("MeleeAttacking", entity);
	}

	return NODE_STATUS::SUCCESS;
}