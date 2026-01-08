#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "application.h"
#include "core/engine/engine.h"
#include <stdexcept>
#include <logging/log_backend.h>
#ifdef _WIN32
#undef APIENTRY
#endif

int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

    Core::Platform::Config platformConfig{
        .displayWidth = 1920,
        .displayHeight = 1080,
        .appName = "MagicGame",
        .enableValidation = false,  // Disable validation for game builds
        .logFilename = "game.log",
        .logToConsole = false,
        .logToFile = true,
        .overwriteLog = true,
        .logLevel = Warning};

    if(!Core::Platform::Get().Initialize(platformConfig)) {
        MessageBoxA(NULL, "Failed to initialize platform", "Initialization Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    {
        Engine<GameApplication> engine;
        engine.Initialize();

        while(engine.ExecuteFrame()) {
            // Game loop
        }

        engine.Shutdown();
    }

    Core::Platform::Get().Shutdown();
    return 0;
}
