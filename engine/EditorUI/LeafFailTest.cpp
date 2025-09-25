#include "LeafFailTest.h"
#include "BehaviourTreeFactory.h"

BT_REGISTER_NODE(LeafFailTest, "LeafFailTest")

NODE_STATUS LeafFailTest::OnUpdate(ecs::EntityHandle entity)
{
	CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for failure";
	return NODE_STATUS::SUCCESS;
}
