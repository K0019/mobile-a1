#include "Engine/BehaviorTree/Range/Range_DecoEnemyInFront.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"
#include "Physics/Physics.h"

NODE_STATUS D_Range_EnemyInFront::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	if (!enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 enemyPos{ entity->GetTransform().GetWorldPosition() };
	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 diff{ playerPos - enemyPos };
	std::vector<physics::RaycastHit> hitInfos{};
	physics::RaycastAll(enemyPos, diff, hitInfos, diff.Length());
	bool inFront{ false };

	for (physics::RaycastHit& hitInfo : hitInfos)
	{
		//If the hit entity is itself, ignore.
		if (hitInfo.entityHit == entity)
			continue;

		if (hitInfo.entityHit->HasComp<EnemyComponent>())
		{
			inFront = true;
			break;
		}
	}

	if (!inFront)
		return NODE_STATUS::SUCCESS;

	NODE_STATUS status{ childPtr->Tick(entity) };
	return status == NODE_STATUS::SUCCESS ? NODE_STATUS::FAILURE : status;
}