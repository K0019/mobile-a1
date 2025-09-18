#include "ComSelector.h"

ComSelector::ComSelector()
    : currentIndex(0)
{
}

void ComSelector::AddChild(BehaviorNode* child)
{
    BaseNode::AddChild(child);
}

void ComSelector::OnEnter()
{
    currentIndex = 0;
    BehaviorNode::OnEnter();
}

void ComSelector::OnUpdate(float dt)
{
    // if any child succeeds, node succeeds
    BehaviorNode* currentNode = children[currentIndex];
    currentNode->Tick(dt);

    if (currentNode->Succeeded() == true)
    {
        CONSOLE_LOG(LEVEL_FATAL) << "This selector is success";
        OnSuccess();
    }
    else if (currentNode->Failed() == true)
    {
        
        // move to the next node
        ++currentIndex;

        //if all children fail, the node fail

        if (currentIndex >= children.size())
        {
            CONSOLE_LOG(LEVEL_FATAL) << "This selector failed";

            OnFailure();
        }
    }
}
