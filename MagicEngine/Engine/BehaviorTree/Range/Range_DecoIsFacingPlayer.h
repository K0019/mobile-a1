#include "Engine/BehaviorTree/BehaviourTree.h"

class D_Range_IsFacingPlayer
	: public Decorator
{
public:
	void OnInitialize() override;
	NODE_STATUS OnUpdate(ecs::EntityHandle entity) override;

private:
	float fieldOfViewAngle;
};