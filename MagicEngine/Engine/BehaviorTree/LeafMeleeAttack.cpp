#include "Engine/BehaviorTree/LeafMeleeAttack.h"
#include "Game/EnemyCharacter.h"
#include "Physics/Collision.h"
#include "Game/Character.h"
#include "Graphics/AnimationComponent.h"

void L_MeleeAttack::OnInitialize()
{
	currSpentTime = 0.f;
}

NODE_STATUS L_MeleeAttack::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	auto animComp{ entity->GetComp<AnimationComponent>() };
	
	characterComp->Attack();

	if (!enemyComp || !characterComp || !animComp || !enemyComp->attackCollider)
		return NODE_STATUS::FAILURE;


	auto attackTrigger{ enemyComp->attackCollider->GetComp<physics::BoxColliderComp>() };
	if (!attackTrigger)
		return NODE_STATUS::FAILURE;

	if (characterComp->currentStunTime > 0.f)
	{
		attackTrigger->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, false);
		return NODE_STATUS::FAILURE;
	}

	if (!attackTrigger->GetFlag(physics::COLLIDER_COMP_FLAG::ENABLED))
		attackTrigger->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, true);

	currSpentTime += GameTime::Dt();

	if (currSpentTime < enemyComp->attackTime)
		return NODE_STATUS::RUNNING;

	attackTrigger->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, false);
	return NODE_STATUS::SUCCESS;
}