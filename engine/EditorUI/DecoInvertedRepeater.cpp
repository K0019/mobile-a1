#include "DecoInvertedRepeater.h"
#include "BehaviourTreeFactory.h"
BT_REGISTER_NODE(DecoratorInverter, "DecoratorInverter")


NODE_STATUS DecoratorInverter::OnUpdate(ecs::EntityHandle entity)
{
	NODE_STATUS stat{ childPtr->Tick(entity) };
	if (stat == NODE_STATUS::SUCCESS)
		return NODE_STATUS::FAILURE;
	else if (stat == NODE_STATUS::FAILURE)
		return NODE_STATUS::SUCCESS;
	else
		return NODE_STATUS::RUNNING;
}
