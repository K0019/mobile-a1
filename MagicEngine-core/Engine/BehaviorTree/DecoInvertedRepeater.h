#pragma once
#include "Engine/BehaviorTree/BehaviourNode.h"

class DecoratorInverter
    : public Decorator
{
protected:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};

