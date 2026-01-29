#include "Range_LeafThrow.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"

void L_Range_Throw::OnInitialize()
{
	waitTimer = 1.f;
}

NODE_STATUS L_Range_Throw::OnUpdate(ecs::EntityHandle entity)
{
	if (waitTimer != 1.f)
	{
		waitTimer -= GameTime::Dt();
		return waitTimer <= 0.f ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
	}

	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	if (!charComp->heldItem || !enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 weaponPos{ charComp->heldItem->GetTransform().GetWorldPosition() };

	charComp->Throw(weaponPos - playerPos);
	waitTimer -= GameTime::Dt();
	return NODE_STATUS::RUNNING;
}