#pragma once
#include "BehaviourNode.h"
#include "Engine/NavMeshAgent.h"

class L_FollowPlayerUsingNavMesh : public BehaviorNode
{
public:
    void OnInitialize() override;
    NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
    navmesh::NavMeshPath path;
    float timer;
    float pathUpdateDuration;
    std::size_t currentCornerIndex;
};