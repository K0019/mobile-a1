#include "ComSelector.h"

ComSelector::ComSelector() : currentIndex(0)
{
}

void ComSelector::onEnter()
{
    currentIndex = 0;
    BehaviorNode::onEnter();
}

void ComSelector::onUpdate(float dt)
{
    // if any child succeeds, node succeeds
    BehaviorNode* currentNode = children[currentIndex];
    currentNode->tick(dt);

    if (currentNode->succeeded() == true)
    {
        CONSOLE_LOG(LEVEL_FATAL) << "This selector is success";
        onSuccess();
    }
    else if (currentNode->failed() == true)
    {
        
        // move to the next node
        ++currentIndex;

        //if all children fail, the node fail

        if (currentIndex >= children.size())
        {
            CONSOLE_LOG(LEVEL_FATAL) << "This selector failed";

            onFailure();
        }
    }
}
