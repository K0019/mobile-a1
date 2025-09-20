#pragma once
#include "BehaviourNode.h"

class ComSelector : public CompositeNode<ComSelector>  
{
public:
    ComSelector();


protected:
    size_t currentIndex;

    virtual void OnEnter() override;
    virtual void OnUpdate(float dt) override;
};