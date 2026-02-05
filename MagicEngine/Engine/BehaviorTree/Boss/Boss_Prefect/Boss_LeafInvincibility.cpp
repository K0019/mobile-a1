#include "Boss_LeafInvincibility.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Boss_Prefect_Util.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Graphics/AnimationComponent.h"
#include "Managers/AudioManager.h"

// Set boss invinc time here, based on Canva values rn so change as balancing requires
float L_Boss_Prefect_Invincibility::invincibilityTime = 1.5f;

void L_Boss_Prefect_Invincibility::OnInitialize()
{
    currentInvincibilityTime = invincibilityTime;
}

NODE_STATUS L_Boss_Prefect_Invincibility::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (!inited)
    {
        ST<AudioManager>::Get()->PlaySound3D("defence stance", false, entity->GetTransform().GetWorldPosition(), AudioType::SFX, std::pair<float, float>{2.0f, 50.0f}, 0.6f);
        if (auto animComp{ entity->GetComp<AnimationComponent>() })
            animComp->TransitionTo(5847024716697507243, 0.1f);
        inited = true;
    }

    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        // Don't move here
        characterComp->SetMovementVector(Vec2{ 0.0f });

    }
    if (auto healthComp{ entity->GetComp<HealthComponent>() })
    {
        auto enemyComp{ entity->GetComp<EnemyComponent>() };
        /*Vec2 dir = Boss_Prefect_Util::GetMovementDirection(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
        Vec3 movementDir3D{ dir.x, 0.0f, dir.y };
		Boss_Prefect_Util::MoveInDirection(entity, movementDir3D);*/
        
        // Use direct direction to player for rotation so boss faces the player
        Vec2 dirToPlayer = Boss_Prefect_Util::GetMovementTowards(entity->GetTransform().GetWorldPosition(), enemyComp->playerReference->GetTransform().GetWorldPosition());
        Boss_Prefect_Util::RotateTowards(entity, dirToPlayer);
        healthComp->SetInvincibleState(HealthComponent::I_INVINCIBLE);
        currentInvincibilityTime -= GameTime::Dt();

        if (currentInvincibilityTime <= 0.0f)
        {
            healthComp->SetInvincibleState(HealthComponent::I_NONE);
            return NODE_STATUS::SUCCESS;
        }
    }
    return NODE_STATUS::RUNNING;
}

void L_Boss_Prefect_Invincibility::OnTerminate(NODE_STATUS)
{
    inited = false;
}
