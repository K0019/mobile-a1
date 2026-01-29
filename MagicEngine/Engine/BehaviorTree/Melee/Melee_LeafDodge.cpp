#include "Melee_LeafDodge.h"
#include "Game/Character.h"
#include "Game/EnemyCharacter.h"

void L_Melee_Dodge::OnInitialize()
{
	probability = 35;
	dodged = false;
}

NODE_STATUS L_Melee_Dodge::OnUpdate(ecs::EntityHandle entity)
{
	auto charComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };

	if (!charComp || !enemyComp)
		return NODE_STATUS::FAILURE;

	//Check stun.
	if (charComp->currentStunTime > 0.f || !enemyComp->playerReference)
		return NODE_STATUS::FAILURE;

	Vec3 playerPos{ enemyComp->playerReference->GetTransform().GetWorldPosition() };
	Vec3 enemyPos{ entity->GetTransform().GetWorldPosition() };

	if (!dodged && probability < (std::rand() % 100) + 1)
		return NODE_STATUS::FAILURE;

	float rotation{ entity->GetTransform().GetWorldRotation().y };
	rotation = std::rand() % 2 ? rotation - 120.f : rotation + 120.f;

	if (charComp->currentDodgeTime <= 0.f && !dodged)
	{
		charComp->Dodge(Vec2{ std::cosf(math::ToRadians(rotation)), std::sinf(math::ToRadians(rotation)) });
		dodged = true;
	}

	return charComp->currentDodgeCooldown <= 0.f ? NODE_STATUS::SUCCESS : NODE_STATUS::RUNNING;
}