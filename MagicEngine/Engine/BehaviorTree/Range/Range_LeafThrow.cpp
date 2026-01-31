#include "Range_LeafThrow.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"
#include "Scripting/ScriptComponent.h"
void L_Range_Throw::OnInitialize()
{
	waitTimer = 1.f;
}

NODE_STATUS L_Range_Throw::OnUpdate(ecs::EntityHandle entity)
{
	if (waitTimer != 1.f)
	{
		waitTimer -= GameTime::Dt();
		if (waitTimer <= 0.f)
		{
			std::vector<ecs::EntityHandle> enemyList{};
			BehaviorTree::globalBlackBoard.GetValue("RangeEnemyList", enemyList);
			if (enemyList.empty())
				return NODE_STATUS::SUCCESS;
			BehaviorTree::globalBlackBoard.SetValue("RangeAttacking", enemyList[std::rand() % enemyList.size()]);
			return NODE_STATUS::SUCCESS;
		}
			
		return NODE_STATUS::RUNNING;
	}

	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	if (!charComp->heldItem || !enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 enemyPos{ entity->GetTransform().GetWorldPosition() };

	auto scriptComp{ charComp->heldItem->GetComp<ScriptComponent>() };
	auto colliderComp{ charComp->heldItem->GetComp<physics::BoxColliderComp>() };
	if (!colliderComp || !scriptComp)
		return NODE_STATUS::FAILURE;

	scriptComp->CallScriptFunction("throw", entity);
	charComp->Throw(playerPos - enemyPos);
	waitTimer -= GameTime::Dt();
	return NODE_STATUS::RUNNING;
}