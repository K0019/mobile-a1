#include "Melee_LeafCheckAttack.h"
#include "Game/Character.h"

NODE_STATUS L_Melee_CheckAttack::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };

	if (!charComp)
		return NODE_STATUS::FAILURE;

	//Check the list of current enemies.
	if (!BehaviorTree::globalBlackBoard.HasKey("MeleeEnemyList"))
		BehaviorTree::globalBlackBoard.SetValue("MeleeEnemyList", std::vector<ecs::EntityHandle>{entity});
	else
	{
		std::vector<ecs::EntityHandle> enemyList{};
		if (!BehaviorTree::globalBlackBoard.GetValue("MeleeEnemyList", enemyList))
			return NODE_STATUS::FAILURE;

		if (std::find(enemyList.begin(), enemyList.end(), entity) == enemyList.end())
		{
			enemyList.push_back(entity);
			BehaviorTree::globalBlackBoard.SetValue("MeleeEnemyList", enemyList);
		}
	}

	//Check stun.
	if (charComp->currentStunTime > 0.f)
	{
		std::vector<ecs::EntityHandle> enemyList{};
		BehaviorTree::globalBlackBoard.GetValue("MeleeEnemyList", enemyList);
		if (enemyList.empty())
			return NODE_STATUS::FAILURE;
		BehaviorTree::globalBlackBoard.SetValue("MeleeAttacking", enemyList[std::rand() % enemyList.size()]);
		return NODE_STATUS::FAILURE;
	}

	//If there's no enemy attacking the player, set the entity to attacking.
	if (!BehaviorTree::globalBlackBoard.HasKey("MeleeAttacking"))
		BehaviorTree::globalBlackBoard.SetValue("MeleeAttacking", entity);
	else
	{
		ecs::EntityHandle handle{};
		if (!BehaviorTree::globalBlackBoard.GetValue("MeleeAttacking", handle))
			return NODE_STATUS::FAILURE;

		if (!ecs::IsEntityHandleValid(handle))
			BehaviorTree::globalBlackBoard.SetValue("MeleeAttacking", entity);
	}

	return NODE_STATUS::SUCCESS;
}