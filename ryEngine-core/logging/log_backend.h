#pragma once
// Define this *before* including any Quill headers
// This prevents Quill from defining LOG_INFO(logger, ...) itself.
#define QUILL_DISABLE_NON_PREFIXED_MACROS 1
#include <string>
#include <string_view>
#include <atomic>
#include <utility> // For std::forward
#include <vector>  // For passing sinks vector
// --- Include Quill Headers ---
// Quill types and namespaces will be visible where LogBackend.h is included.
#include <quill/Logger.h>
#include <quill/LogMacros.h>

// --- Configuration Structure ---
enum EngineLogLevel : uint8_t
{
  Trace,
  // Map to Quill TraceL3 generally
  Debug,
  Info,
  Warning,
  Error,
  Critical,
  None // Map to Quill None
};

inline quill::LogLevel MapEngineLogLevelToQuill(EngineLogLevel level)
{
  switch (level)
  {
  // Map Trace to a specific Quill trace level (e.g., L3)
  case Trace:
    return quill::LogLevel::TraceL3;
  case Debug:
    return quill::LogLevel::Debug;
  case Info:
    return quill::LogLevel::Info;
  case Warning:
    return quill::LogLevel::Warning;
  case Error:
    return quill::LogLevel::Error;
  case Critical:
    return quill::LogLevel::Critical;
  case None:
    return quill::LogLevel::None;
  default:
    return quill::LogLevel::Info; // Sensible default
  }
}

inline const char* MapEngineLogLevelToString(EngineLogLevel level)
{
  switch (level)
  {
  case Trace:
    return "Trace";
  case Debug:
    return "Debug";
  case Info:
    return "Info";
  case Warning:
    return "Warning";
  case Error:
    return "Error";
  case Critical:
    return "Critical";
  case None:
    return "None";
  default:
    return "Unknown";
  }
}

struct LogConfig
{
  std::string filename = "engine.log";
  bool logToConsole = true;
  bool logToFile = true;
  bool overwriteFile = true; // If false, appends ('a' mode)
  EngineLogLevel logLevelFilter = Info;
};

// --- Static Backend Management Class ---
class LogBackend
{
public:
  LogBackend() = delete;

  ~LogBackend() = delete;

  // Call once at the start of the application
  static bool initialize(const LogConfig& config = {});

  // Call once at the end of the application
  static void shutdown();

  // Returns the pointer to the single engine logger instance.
  // Returns nullptr if not initialized. Safe to call anytime.
  [[nodiscard]] static quill::Logger* getLogger();

  // Checks if the backend has been successfully initialized.
  [[nodiscard]] static bool isInitialized();

private:
  // C++17 inline static members (define in .cpp for C++14 or earlier)
  inline static std::atomic<bool> g_initialized = false;
  inline static quill::Logger* g_logger = nullptr;
  inline static std::string g_loggerName = "d"; // default name
};

// --- RAII Wrapper ---
class ScopedLogger
{
public:
  explicit ScopedLogger(const LogConfig& config = {}) { LogBackend::initialize(config); }

  ~ScopedLogger() { LogBackend::shutdown(); }

  ScopedLogger(const ScopedLogger&) = delete;

  ScopedLogger& operator=(const ScopedLogger&) = delete;

  ScopedLogger(ScopedLogger&&) = delete;

  ScopedLogger& operator=(ScopedLogger&&) = delete;
};
