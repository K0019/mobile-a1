#pragma once
#include <string>
#include <string_view>
#include <atomic>
#include <utility>
#include <memory>

// Forward-declare spdlog::logger to avoid pulling in heavy headers everywhere
namespace spdlog { class logger; }

// --- Configuration Structure ---
enum EngineLogLevel : uint8_t
{
  Trace,
  Debug,
  Info,
  Warning,
  Error,
  Critical,
  None
};

// Maps our engine log level enum to spdlog level values (spdlog::level::level_enum).
// Returns int to avoid including spdlog headers here; cast to spdlog::level::level_enum at use site.
inline int MapEngineLogLevelToSpdlog(EngineLogLevel level)
{
  // spdlog levels: trace=0, debug=1, info=2, warn=3, err=4, critical=5, off=6
  switch (level)
  {
  case Trace:    return 0; // spdlog::level::trace
  case Debug:    return 1; // spdlog::level::debug
  case Info:     return 2; // spdlog::level::info
  case Warning:  return 3; // spdlog::level::warn
  case Error:    return 4; // spdlog::level::err
  case Critical: return 5; // spdlog::level::critical
  case None:     return 6; // spdlog::level::off
  default:       return 2; // spdlog::level::info
  }
}

inline const char* MapEngineLogLevelToString(EngineLogLevel level)
{
  switch (level)
  {
  case Trace:    return "Trace";
  case Debug:    return "Debug";
  case Info:     return "Info";
  case Warning:  return "Warning";
  case Error:    return "Error";
  case Critical: return "Critical";
  case None:     return "None";
  default:       return "Unknown";
  }
}

struct LogConfig
{
  std::string filename = "engine.log";
  bool logToConsole = true;
  bool logToFile = true;
  bool overwriteFile = true;
  EngineLogLevel logLevelFilter = Info;
};

// --- Static Backend Management Class ---
class LogBackend
{
public:
  LogBackend() = delete;
  ~LogBackend() = delete;

  static bool initialize(const LogConfig& config = {});
  static void shutdown();

  // Returns raw pointer for performance (avoids atomic ref-count on every log call).
  // Returns nullptr if not initialized.
  [[nodiscard]] static spdlog::logger* getLogger();

  [[nodiscard]] static bool isInitialized();

private:
  inline static std::atomic<bool> g_initialized = false;
  inline static std::shared_ptr<spdlog::logger> g_logger = nullptr;
  inline static std::string g_loggerName = "d";
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
