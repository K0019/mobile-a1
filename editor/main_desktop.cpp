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
#ifdef _DEBUG
#include <cstdio>    // For FILE, freopen_s
#include <iostream>  // For std::cout etc.
#include <fcntl.h>   // For _O_TEXT (optional)

class DebugConsole
{
  public:
  DebugConsole()
  {
    if(AllocConsole()) // Check if allocation succeeded
    {
      FILE* pCout;
      FILE* pCerr;
      FILE* pCin;
      freopen_s(&pCout, "CONOUT$", "w", stdout);
      freopen_s(&pCerr, "CONOUT$", "w", stderr);
      freopen_s(&pCin, "CONIN$", "r", stdin);
      std::cout.clear(); // Clear potential error flags
      std::cerr.clear();
      std::cin.clear();
      std::cout.sync_with_stdio(true);
      std::cerr.sync_with_stdio(true);
      HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD dwMode = 0;
      GetConsoleMode(hOut, &dwMode);
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING // Define if SDK is old
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
      dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
      SetConsoleMode(hOut, dwMode);
      system("cls");
    }
    else
    {
      throw std::runtime_error("Failed to allocate console");
    }
  }

  ~DebugConsole()
  {
    FreeConsole();
  }
};
#endif // _DEBUG

int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance, [[maybe_unused]] HINSTANCE hPrevInstance, [[maybe_unused]] LPSTR lpCmdLine, [[maybe_unused]] int nCmdShow)
{
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#ifdef _DEBUG
  DebugConsole debugConsole;
#endif

  Core::Platform::Config platformConfig{
    .displayWidth = 1920,
    .displayHeight = 1080,
    .appName = "MagicEngine",
    .enableValidation = true,
    .logFilename = "engine.log",
    .logToConsole = true,
    .logToFile = false,
    .overwriteLog = true,
    .logLevel = Trace};
  if(!Core::Platform::Get().Initialize(platformConfig)) {
    MessageBoxA(NULL, "Failed to initialize platform", "Initialization Error", MB_OK | MB_ICONERROR);
    return 1;
  }
  {
    // Inject runtime env variables for vulkan validation layers, if you need this when running the exe direct
    _putenv_s("VK_ADD_LAYER_PATH", "vulkan");

    Engine<Application> engine;
    // Initialize engine and application
    engine.Initialize();

    // Main loop - desktop owns the loop
    while(engine.ExecuteFrame()) {
      // Loop continues until ExecuteFrame returns false
    }
    // Shutdown
    engine.Shutdown();
  }
  Core::Platform::Get().Shutdown();

  return 0;
}
