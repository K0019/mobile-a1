#pragma once
#include "../../BehaviourNode.h"

class L_Boss_Prefect_BookingSlips : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    static float burstDelay;
    static float attackDistance;  // Distance check before starting burst sequence
    static int burstCount;

    float currentBurstDelay;
    int currentBurstCount;
    bool hasStartedBurst;  // Track if burst sequence has begun (distance check only applies before this)
};