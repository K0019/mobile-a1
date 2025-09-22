#include "LeafKeyPressed.h"
#include "BehaviourTreeFactory.h"
BT_REGISTER_NODE(L_CheckMouseClick, "L_CheckMouseClick")


NODE_STATUS L_CheckMouseClick::OnUpdate()
{
    CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for key press sorta";
    return NODE_STATUS::SUCCESS;
}
