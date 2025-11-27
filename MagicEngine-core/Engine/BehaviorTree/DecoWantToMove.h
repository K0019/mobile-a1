#pragma once
#include "Engine/BehaviorTree/BehaviourNode.h"

class D_WantToMove
    : public Decorator
{
protected:
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;
};
