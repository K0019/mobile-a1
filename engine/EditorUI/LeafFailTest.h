#pragma once
#include "BehaviourNode.h"

class LeafFailTest 
    : public BehaviorNode
{
protected:
    NODE_STATUS OnUpdate() override;
};