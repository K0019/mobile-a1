#pragma once
#include "../../BehaviourNode.h"

// Behaves like a sequencer, but with a hard delay when changing to the next child node.

class S_Boss_TransitionPhases : public CompositeNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
private:
    BehaviorNodes::iterator currentChildItr;
    float transitionDelay;
    float currentTransitionDelay;
};