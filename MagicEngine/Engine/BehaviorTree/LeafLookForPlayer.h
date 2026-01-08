#pragma once
#include "BehaviourNode.h"

class L_LookForPlayer 
    : public BehaviorNode
{
public:
    void OnInitialize() override {}
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};