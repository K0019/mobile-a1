#include "LeafFailTest.h"
#include "BehaviourTreeFactory.h"

BT_REGISTER_NODE(LeafFailTest, "LeafFailTest")

NODE_STATUS LeafFailTest::OnUpdate()
{
	CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for failure";
	return NODE_STATUS::SUCCESS;
}
