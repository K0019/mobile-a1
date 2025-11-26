#include "Boss_LeafLashOut.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

// Set boss invinc time here, based on Canva values rn so change as balancing requires

void L_Boss_Prefect_LashOut::OnInitialize()
{
}

NODE_STATUS L_Boss_Prefect_LashOut::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto characterComp{ entity->GetComp<CharacterMovementComponent>() })
    {
        // Don't move here
        characterComp->SetMovementVector(Vec2{ 0.0f });
    }
    if (auto healthComp{ entity->GetComp<HealthComponent>() })
    {
    }
    return NODE_STATUS::RUNNING;
}