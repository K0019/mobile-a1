#include "application.h"
#include <core/engine/engine.h>
#include "Engine/Engine.h"

void GameApplication::Initialize(Context& context)
{
    magicEngine.Init(context);
    // No editor panels - just run the game
}

void GameApplication::Update([[maybe_unused]] Context& context, FrameData& frame)
{
    magicEngine.ExecuteFrame(frame);
}

void GameApplication::Shutdown([[maybe_unused]] Context& context)
{
    magicEngine.shutdown();
}
