#pragma once
#include "Engine/BehaviorTree/BehaviourTree.h"

class L_FollowThrough
	: public BehaviorNode
{
public:
	void OnInitialize();
	NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
	float currSpentTime;
};