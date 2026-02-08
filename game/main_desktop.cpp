#if !defined(NOMINMAX)
#define NOMINMAX
#endif
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <cstdio>
#include "application.h"
#include "core/engine/engine.h"
#include <stdexcept>
#include <logging/log_backend.h>
#ifdef _WIN32
#undef APIENTRY
#endif

// Allocate a debug console for viewing log output
static void AllocateDebugConsole() {
    if (AllocConsole()) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
        freopen_s(&fp, "CONIN$", "r", stdin);
        SetConsoleTitleA("MagicGame Debug Console");
        printf("Debug console initialized\n");
    }
}

int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    // Enable debug console for troubleshooting
#ifdef DEBUG
    AllocateDebugConsole();
    Core::Platform::Config platformConfig{
        .displayWidth = 1920,
        .displayHeight = 1080,
        .appName = "MagicGame",
        .enableValidation = false,  // Disable validation for game builds
        .logFilename = "game.log",
        .logToConsole = true,   // Enable console logging for debug
        .logToFile = true,
        .overwriteLog = true,
        .logLevel = Trace};     // Show all log levels for debugging
#else
    Core::Platform::Config platformConfig{
        .displayWidth = 1920,
        .displayHeight = 1080,
        .appName = "MagicGame",
        .enableValidation = false,  // Disable validation for game builds
        .logFilename = "game.log",
        .logToConsole = false,   // Enable console logging for debug
        .logToFile = false,
        .overwriteLog = true,
        .logLevel = None};     // Show all log levels for debugging
#endif

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
