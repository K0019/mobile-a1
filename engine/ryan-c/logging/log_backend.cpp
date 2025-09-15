#include "log_backend.h"
// --- Quill Includes (Based on example and v2 structure) ---
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h> // Include FileSink
// #include "quill/PatternFormatter.h" // Uncomment if setting custom patterns
#include <vector>
#include <iostream> // For critical init errors
#include <atomic>
#include <memory> // For std::unique_ptr if managing sinks manually (less common now)

// --- Static Member Definitions ---
bool LogBackend::initialize(const LogConfig& config)
{
  bool expected = false;
  if (!g_initialized.compare_exchange_strong(expected, true))
  {
    // Already initialized or initialization in progress
    std::cerr << "LogBackend::initialize called multiple times." << std::endl;
    // Wait until initialization completes if called concurrently (best effort)
    while (!isInitialized() && getLogger() == nullptr)
    {
      /* spin wait */
    }
    return isInitialized(); // Return current state
  }
  try
  {
    // 1. Start the backend thread
    quill::Backend::start(); // Default config is often sufficient
    // 2. Create Sinks
    std::vector<std::shared_ptr<quill::Sink>> sinks_for_logger;
    if (config.logToConsole)
    {
      auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink");
      // Optional: Configure console_sink here (e.g., set_pattern)
      sinks_for_logger.push_back(console_sink);
    }
    if (config.logToFile && !config.filename.empty())
    {
      quill::FileSinkConfig file_sink_config;
      file_sink_config.set_open_mode(config.overwriteFile ? 'w' : 'a');
      auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(config.filename, file_sink_config);
      // Optional: Configure file_sink here
      sinks_for_logger.push_back(file_sink);
    }
    if (sinks_for_logger.empty())
    {
      std::cerr << "LogBackend Warning: No sinks configured. Logging will be silent." << std::endl;
    }
    std::string format_pattern = "%(time) [%(thread_id)]:%(short_source_location:<28):%(caller_function) %(log_level:<9)%(tags) %(message)";
    std::string time_format = "%H:%M:%S.%Qms";             // ** Concise Timestamp **
    quill::Timezone timezone = quill::Timezone::LocalTime; // Or GmtTime if preferred
    quill::PatternFormatterOptions formatter_options{format_pattern, time_format, timezone};
    // Create logger with the vector of sinks and the custom formatter options
    g_logger = quill::Frontend::create_or_get_logger(g_loggerName, std::move(sinks_for_logger), formatter_options);
    if (!g_logger)
    {
      // If logger creation failed, reset state and report error
      throw std::runtime_error("Failed to create or get Quill logger");
    }
    // 4. Configure the Logger instance
    g_logger->set_log_level(MapEngineLogLevelToQuill(config.logLevelFilter));
    // Log initialization success *using the newly defined macros*
    QUILL_LOG_INFO(g_logger, "LogBackend initialized. Filter Level: {}", MapEngineLogLevelToString(config.logLevelFilter));
    // Final check in case something went wrong after setting g_logger
    if (!g_logger) { throw std::runtime_error("Logger pointer became null unexpectedly during initialization."); }
    return true;
  }
  catch (const std::exception& e)
  {
    std::cerr << "LogBackend Critical Error during initialization: " << e.what() << std::endl;
    g_logger = nullptr;    // Ensure logger is null on failure
    g_initialized = false; // Reset flag on failure
    // Consider if quill::Backend::stop() is safe to call here
    return false;
  }
}

void LogBackend::shutdown()
{
  bool expected = true;
  // Only proceed if it was initialized
  if (g_initialized.compare_exchange_strong(expected, false))
  {
    // Use the macros one last time before shutdown
    QUILL_LOG_INFO(g_logger, "LogBackend shutting down...");
    // Stop the backend thread - this blocks until flushed and stopped
    quill::Backend::stop();

    // Clear the logger pointer AFTER stopping the backend might be slightly safer,
    // though macros already check for null. Resetting state is good practice.
    g_logger = nullptr;
  }
  // If already shut down or never initialized, do nothing.
}

// Public getter for the logger pointer
quill::Logger* LogBackend::getLogger()
{
  // Ensure reads see the latest value, especially during/after init/shutdown
  return g_initialized.load() ? g_logger : nullptr;
}

// Public check for initialization status
bool LogBackend::isInitialized()
{
  // Read the atomic flag
  return g_initialized.load();
}
