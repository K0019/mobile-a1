#pragma once
#include "BehaviourNode.h"

class LeafFailTest : public BaseNode<LeafFailTest>
{
protected:
    virtual void OnUpdate(float dt);
};