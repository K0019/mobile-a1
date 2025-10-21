#include "log_backend.h"
// --- Quill Includes (Based on example and v2 structure) ---
#include <quill/Backend.h>
#include <quill/Frontend.h>
#include <quill/LogMacros.h>
#include <quill/Logger.h>
#include <quill/sinks/ConsoleSink.h>
#include <quill/sinks/FileSink.h> // Include FileSink
#if defined(__ANDROID__)
#include <quill/sinks/AndroidSink.h>
#endif
// #include "quill/PatternFormatter.h" // Uncomment if setting custom patterns
#include <vector>
#include <iostream> // For critical init errors
#include <atomic>
#include <memory> // For std::unique_ptr if managing sinks manually (less common now)

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

    // 1. Start the backend thread
    quill::Backend::start(); // Default config is often sufficient

    // 2. Create Sinks
    std::vector<std::shared_ptr<quill::Sink>> sinks_for_logger;
    
    if (config.logToConsole)
    {
#ifdef ANDROID
        // Use AndroidSink for console output on Android
        auto console_sink = quill::Frontend::create_or_get_sink<quill::AndroidSink>("console_sink", [](){
            quill::AndroidSinkConfig asc;
            asc.set_tag("ryEngine");
            asc.set_format_message(true);
            return asc;
        }());
#else
        auto console_sink = quill::Frontend::create_or_get_sink<quill::ConsoleSink>("console_sink");
#endif
        if (!console_sink)
        {
            std::cerr << "LogBackend Error: Failed to create console sink." << std::endl;
            g_initialized = false;
            return false;
        }
        sinks_for_logger.push_back(console_sink);
    }

    if (config.logToFile && !config.filename.empty())
    {
        quill::FileSinkConfig file_sink_config;
        file_sink_config.set_open_mode(config.overwriteFile ? 'w' : 'a');
        auto file_sink = quill::Frontend::create_or_get_sink<quill::FileSink>(config.filename, file_sink_config);
        
        if (!file_sink)
        {
            std::cerr << "LogBackend Error: Failed to create file sink for: " << config.filename << std::endl;
            g_initialized = false;
            return false;
        }
        sinks_for_logger.push_back(file_sink);
    }

    if (sinks_for_logger.empty())
    {
        std::cerr << "LogBackend Warning: No sinks configured. Logging will be silent." << std::endl;
    }

    // 3. Configure formatter
    std::string format_pattern = "%(time) [%(thread_id)]:%(short_source_location:<28):%(caller_function) %(log_level:<9)%(tags) %(message)";
    std::string time_format = "%H:%M:%S.%Qms";             // ** Concise Timestamp **
    quill::Timezone timezone = quill::Timezone::LocalTime; // Or GmtTime if preferred

#ifdef ANDROID
    // Use System clock for Android compatibility
    quill::PatternFormatterOptions formatter_options{format_pattern, time_format, timezone};
    g_logger = quill::Frontend::create_or_get_logger(g_loggerName, std::move(sinks_for_logger), 
                                                   formatter_options, quill::ClockSourceType::System);
#else
    quill::PatternFormatterOptions formatter_options{format_pattern, time_format, timezone};
    g_logger = quill::Frontend::create_or_get_logger(g_loggerName, std::move(sinks_for_logger), formatter_options);
#endif

    if (!g_logger)
    {
        std::cerr << "LogBackend Critical Error: Failed to create or get Quill logger" << std::endl;
        g_initialized = false;
        return false;
    }

    // 4. Configure the Logger instance
    g_logger->set_log_level(MapEngineLogLevelToQuill(config.logLevelFilter));

    // Log initialization success
    QUILL_LOG_INFO(g_logger, "LogBackend initialized. Filter Level: {}", MapEngineLogLevelToString(config.logLevelFilter));

    // Final validation
    if (!g_logger)
    {
        std::cerr << "LogBackend Critical Error: Logger pointer became null unexpectedly during initialization." << std::endl;
        g_initialized = false;
        return false;
    }

    return true;
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
