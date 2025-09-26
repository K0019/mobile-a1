#pragma once
#include "BehaviourNode.h"

class L_CheckMouseClick 
    : public BehaviorNode
{
protected:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};