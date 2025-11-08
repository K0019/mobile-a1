#include "application.h"
#include <core/engine/engine.h>
#include "Engine/Engine.h"

void Application::Initialize(Context& context)
{
    magicEngine.Init(context);
}

void Application::Update([[maybe_unused]] Context& context, FrameData& frame)
{
    magicEngine.ExecuteFrame(frame);
}

void Application::Shutdown([[maybe_unused]] Context& context)
{
    magicEngine.shutdown();
}
