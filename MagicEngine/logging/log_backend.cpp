#include "log_backend.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/null_sink.h>
#if defined(__ANDROID__)
#include <spdlog/sinks/android_sink.h>
#endif
#include <vector>
#include <iostream>
#include <atomic>
#include <memory>

bool LogBackend::initialize(const LogConfig& config)
{
  bool expected = false;
  if (!g_initialized.compare_exchange_strong(expected, true))
  {
    std::cerr << "LogBackend::initialize called multiple times." << std::endl;
    while (!isInitialized() && getLogger() == nullptr)
    {
      /* spin wait */
    }
    return isInitialized();
  }

  // spdlog is synchronous — no backend thread to start.

  std::vector<spdlog::sink_ptr> sinks;

  if (config.logToConsole)
  {
#if defined(__ANDROID__)
    auto console_sink = std::make_shared<spdlog::sinks::android_sink_mt>("ryEngine");
#else
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif
    sinks.push_back(console_sink);
  }

  if (config.logToFile && !config.filename.empty())
  {
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
      config.filename, config.overwriteFile);
    sinks.push_back(file_sink);
  }

  // Bug fix: when no sinks are configured (e.g. Release with logToConsole=false, logToFile=false),
  // use a null sink instead of leaving the vector empty. This prevents crashes.
  if (sinks.empty())
  {
    sinks.push_back(std::make_shared<spdlog::sinks::null_sink_mt>());
  }

  g_logger = std::make_shared<spdlog::logger>(g_loggerName, sinks.begin(), sinks.end());

  // Set log pattern: [HH:MM:SS.mmm] [thread] [LEVEL] message
  g_logger->set_pattern("[%H:%M:%S.%e] [%t] [%^%l%$] %v");

  // Set log level filter
  g_logger->set_level(static_cast<spdlog::level::level_enum>(MapEngineLogLevelToSpdlog(config.logLevelFilter)));

  // Register so spdlog::drop_all() can find it
  spdlog::register_logger(g_logger);

  g_logger->info("LogBackend initialized. Filter Level: {}", MapEngineLogLevelToString(config.logLevelFilter));

  return true;
}

void LogBackend::shutdown()
{
  bool expected = true;
  if (g_initialized.compare_exchange_strong(expected, false))
  {
    if (g_logger)
    {
      g_logger->info("LogBackend shutting down...");
      g_logger->flush();
    }
    spdlog::drop(g_loggerName);
    g_logger.reset();
  }
}

spdlog::logger* LogBackend::getLogger()
{
  return g_initialized.load() ? g_logger.get() : nullptr;
}

bool LogBackend::isInitialized()
{
  return g_initialized.load();
}
