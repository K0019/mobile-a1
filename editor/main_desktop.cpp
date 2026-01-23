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
#include <cstdio>
#include <crtdbg.h>

static void setupDebugEnvironment()
{
  // Enable ANSI escape codes for colored output
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  DWORD dwMode = 0;
  GetConsoleMode(hOut, &dwMode);
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
  dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
  SetConsoleMode(hOut, dwMode);

  // Disable buffering so output appears immediately
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  // === Suppress ALL error dialogs (assert, abort, WER) ===
  // 1. Windows Error Reporting: suppress OS-level crash dialog
  SetErrorMode(GetErrorMode() | SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS);

  // 2. CRT assert(): redirect to stderr instead of dialog box
  _set_error_mode(_OUT_TO_STDERR);

  // 3. CRT abort(): suppress its dialog and WER integration
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

  // 4. CRT debug macros (_ASSERT, _ASSERTE, etc): redirect to stderr
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
}
#endif // _DEBUG

int main([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
#ifdef _DEBUG
  setupDebugEnvironment();
#endif

  Core::Platform::Config platformConfig{
    .displayWidth = 1920,
    .displayHeight = 1080,
    .appName = "MagicEngine",
    .enableValidation = true,
    .logFilename = "engine.log",
    .logToConsole = true,
    .logToFile = true,
    .overwriteLog = true,
    .logLevel = Trace};
  if(!Core::Platform::Get().Initialize(platformConfig)) {
    fprintf(stderr, "Failed to initialize platform\n");
    return 1;
  }
  {
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

#if defined(_WIN32)
int WINAPI WinMain([[maybe_unused]] HINSTANCE hInstance,
                   [[maybe_unused]] HINSTANCE hPrevInstance,
                   [[maybe_unused]] LPSTR lpCmdLine,
                   [[maybe_unused]] int nCmdShow)
{
  return main(__argc, __argv);
}
#endif
