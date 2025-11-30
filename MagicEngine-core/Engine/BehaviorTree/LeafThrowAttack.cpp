#include "Engine/BehaviorTree/LeafThrowAttack.h"
#include "Game/EnemyCharacter.h"
#include "Physics/Collision.h"
#include "Physics/Physics.h"
#include "Game/Character.h"
#include "Engine/PrefabManager.h"
#include "graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

void L_ThrowAttack::OnInitialize()
{
	currSpentTime = 0.f;
}

NODE_STATUS L_ThrowAttack::OnUpdate(ecs::EntityHandle entity)
{
	auto characterComp{ entity->GetComp<CharacterMovementComponent>() };
	auto enemyComp{ entity->GetComp<EnemyComponent>() };
	auto animComp{ entity->GetComp<AnimationComponent>() };
	if (!enemyComp || !characterComp || !animComp)
		return NODE_STATUS::FAILURE;

	if (characterComp->currentStunTime > 0.f)
	{
		//characterComp->isAttacking = false;
		return NODE_STATUS::FAILURE;
	}

	currSpentTime += GameTime::Dt();

	if (currSpentTime < enemyComp->attackTime)
		return NODE_STATUS::RUNNING;

	Vec3 spawnPos{ entity->GetTransform().GetWorldPosition() };
	spawnPos.y += 1.f;

	ST<Scheduler>::Get()->Add([spawnPos, enemyComp]() {
		ecs::EntityHandle attackObj = ST<PrefabManager>::Get()->LoadPrefab("throw");
		ST<AudioManager>::Get()->PlaySound3D("enemy female throwing " + std::to_string(randomRange(1, 5)), false, spawnPos, AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 1.0f);
		attackObj->GetTransform().SetWorldPosition(spawnPos);
	});
	return NODE_STATUS::SUCCESS;
}