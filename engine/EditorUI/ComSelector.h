#pragma once
#include "BehaviourNode.h"

class ComSelector : public BaseNode<ComSelector>
{
public:
    ComSelector();
    void AddChild(BehaviorNode* child);


protected:
    size_t currentIndex;

    virtual void OnEnter() override;
    virtual void OnUpdate(float dt) override;
};