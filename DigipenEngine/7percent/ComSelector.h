#pragma once
#include "BehaviourNode.h"

class ComSelector : public BaseNode<ComSelector>
{
public:
    ComSelector();
    void AddChild(BehaviorNode* child) { addChild(child); }  // public wrapper


protected:
    size_t currentIndex;

    virtual void onEnter() override;
    virtual void onUpdate(float dt) override;
};