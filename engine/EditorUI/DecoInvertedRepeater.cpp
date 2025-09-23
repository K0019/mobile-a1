#include "DecoInvertedRepeater.h"
BT_REGISTER_NODE(DecoratorInverter, "DecoratorInverter")


NODE_STATUS DecoratorInverter::OnUpdate()
{
 //   CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for key press sorta";
    return NODE_STATUS::SUCCESS;
}
