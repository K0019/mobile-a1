#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_Detention : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    static float triggerDistanceSqr;
    static float triggerDistance;
    static float burstDelay;
    static int burstCount;

    float currentBurstDelay;
    int currentBurstCount;
};