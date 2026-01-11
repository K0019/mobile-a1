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
    if (entity)
    {
        while (currentChildItr != childrenPtr.end())
        {
            //Run the child node.
            NODE_STATUS stat{ (*currentChildItr)->Tick(entity) };

            //If the child returns failure or running, return the same thing.
            if (stat != NODE_STATUS::SUCCESS)
                return stat;

            //If the child succeeded, go to the next node.
            ++currentChildItr;
        }
        //If all the node succeeded, return success.
        return NODE_STATUS::SUCCESS;
    }
    return NODE_STATUS::FAILURE;
}