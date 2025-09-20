#include "LeafFailTest.h"

void LeafFailTest::OnUpdate(float dt)
{

    //std::this_thread::sleep_for(std::chrono::seconds(1));

    CONSOLE_LOG(LEVEL_INFO) << "Running leaf node for failure";

    OnFailure();
    

}
