#include "Selector_BossTransitionPhases.h"
#include "../../BehaviourTreeFactory.h"
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"

void S_Boss_TransitionPhases::OnInitialize()
{
    transitionDelay = 2.0f;
    currentTransitionDelay = transitionDelay;
    currentChildItr = childrenPtr.begin();
}

NODE_STATUS S_Boss_TransitionPhases::OnUpdate([[maybe_unused]] ecs::EntityHandle entity)
{
    // Sanic Tea
    if (!entity)
        return NODE_STATUS::FAILURE;

    // Once we reach the end of the list, return success
    if (currentChildItr == childrenPtr.end())
        return NODE_STATUS::SUCCESS;

    // If in transition preiod, we just return RUNNING and decrease transition time
    if (currentTransitionDelay > 0.0f)
    {
        currentTransitionDelay -= GameTime::Dt();
        return NODE_STATUS::RUNNING;
    }

    // Tick current child
    NODE_STATUS stat = (*currentChildItr)->Tick(entity);

    // If not success, just return it
    if (stat != NODE_STATUS::SUCCESS)
    {
        return stat;
    }

    // Child succeeded, start transition delay and ++ current child
    ++currentChildItr;
    currentTransitionDelay = transitionDelay;

    return NODE_STATUS::RUNNING;
}
