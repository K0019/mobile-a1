#include "LeafKeyPressed.h"
#include "BehaviourTreeFactory.h"
BT_REGISTER_NODE(L_CheckMouseClick, "L_CheckMouseClick")


void L_CheckMouseClick::OnUpdate(float dt)
{
    //const auto testKeyPressed = Input::GetKeyPressed(KEY::F2);

    if (true)
    {

     //  CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for key press sorta";

        OnSuccess();
    }
    else
    {
        OnFailure();
    }

}
