#include "LeafKeyPressed.h"
#include "Input.h"

void L_CheckMouseClick::OnUpdate(float dt)
{
    //const auto testKeyPressed = Input::GetKeyPressed(KEY::F2);

    if (true)
    {

        CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for key press sorta";

        OnSuccess();
    }
    else
    {
        OnFailure();
    }

}
