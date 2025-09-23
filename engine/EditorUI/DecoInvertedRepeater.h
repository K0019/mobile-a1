#pragma once
#include "BehaviourNode.h"
#include "BehaviourTreeFactory.h"

class DecoratorInverter
    : public Decorator
{
protected:
    NODE_STATUS OnUpdate() override;
};

