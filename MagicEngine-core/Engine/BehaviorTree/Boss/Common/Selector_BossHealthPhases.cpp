#include "Selector_BossHealthPhases.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

void S_Boss_HealthPhases::OnInitialize()
{
}

NODE_STATUS S_Boss_HealthPhases::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    if (auto healthComp{ entity->GetComp<HealthComponent>() })
    {
        size_t childIndex = (healthComp->GetCurrHealthNormalized() * (float)childrenPtr.size());

        // Account for edge case of this being maxed out
        if (childIndex == childrenPtr.size())
            --childIndex;

        childrenPtr[childIndex]->Tick(entity);
    }

    // We never exit this node
    return NODE_STATUS::RUNNING;
}