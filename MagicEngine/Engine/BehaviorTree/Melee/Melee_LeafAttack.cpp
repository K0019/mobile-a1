#include "Melee_LeafAttack.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"

void L_Melee_Attack::OnInitialize()
{
	attackCoolDown = 0.5f;
}

NODE_STATUS L_Melee_Attack::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };

	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	//Check stun.
	if (charComp->currentStunTime > 0.f || !enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 enemyPos{ entity->GetTransform().GetWorldPosition()};

	if (attackCoolDown == 0.5f)
		charComp->Attack();

	attackCoolDown -= GameTime::Dt();
	return attackCoolDown >= 0.f ? NODE_STATUS::RUNNING : NODE_STATUS::SUCCESS;
}