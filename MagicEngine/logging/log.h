#pragma once

// When building as part of AssetCompiler, use simple stub logging
#ifdef ASSET_COMPILER_BUILD
#include <iostream>
#include <fmt/format.h>

#define LOG_TRACE_L3(fmtstr, ...) ((void)0)
#define LOG_TRACE_L2(fmtstr, ...) ((void)0)
#define LOG_TRACE_L1(fmtstr, ...) ((void)0)
#define LOG_DEBUG(fmtstr, ...) do { std::cout << "[DEBUG] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)
#define LOG_INFO(fmtstr, ...) do { std::cout << "[INFO] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)
#define LOG_NOTICE(fmtstr, ...) do { std::cout << "[NOTICE] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)
#define LOG_WARNING(fmtstr, ...) do { std::cerr << "[WARNING] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)
#define LOG_ERROR(fmtstr, ...) do { std::cerr << "[ERROR] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)
#define LOG_CRITICAL(fmtstr, ...) do { std::cerr << "[CRITICAL] " << fmt::format(fmtstr, ##__VA_ARGS__) << std::endl; } while(false)

#else // Full Quill logging for MagicEngine

#include "log_backend.h"
/*
 * ==========================================
 * Full Macro Guide
 * ==========================================
 *
 * Allows compile-time filtering of log messages to completely compile out log levels,
 * resulting in zero-cost logging.
 *
 * Macros like LOG_TRACE_L3(..), LOG_TRACE_L2(..) will expand to empty statements,
 * reducing branches in compiled code and the number of MacroMetadata constexpr instances.
 *
 * The default value of -1 enables all log levels.
 * Specify a logging level to disable all levels equal to or higher than the specified level.
 *
 * For example to only log warnings and above you can use:
 *   add_compile_definitions(-DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 * or
 *   target_compile_definitions(${TARGET} PRIVATE -DQUILL_COMPILE_ACTIVE_LOG_LEVEL=QUILL_COMPILE_ACTIVE_LOG_LEVEL_WARNING)
 *
 * The original // docs can be found here. https://quillcpp.readthe// docs.io/en/latest/logging_macros.html
 *
 * -- Core Concepts --
 *
 * 1. Formatting Engine:
 * - `LOG_...`: Standard {fmt} formatting.
 * - `LOGV_...`: automatically prints variable names and values without explicitly specifying
 *    each variable name or using {} placeholders in the format string
 * - `LOGJ_...`: Structured JSON logging.
 *    Log via the convenient LOGJ_ macros
 *    LOGJ_INFO("A json message", var_a, var_b)
 *    Or manually specify the desired names of each variable
 *    LOG_INFO("A json message with {var_1} and {var_2}", var_a, var_b);
 *
 * 2. Severity Levels:
 * - Macros are typically named `<TYPE>_<LEVEL>...`.
 * - Standard Levels (increasing severity):
 * - TRACE_L3, TRACE_L2, TRACE_L1 (Fine-grained debugging)
 * - DEBUG (Debugging information)
 * - INFO (Informational messages)
 * - NOTICE (Normal but significant conditions)
 * - WARNING (Warning conditions)
 * - ERROR (Error conditions)
 * - CRITICAL (Critical conditions)
 * - Special Levels:
 * - BACKTRACE (Logs a message *and* a stack backtrace)
 * - DYNAMIC (Allows specifying the log level at runtime)
 *
 * -- Macro Families Explained --
 *
 * The macros generally follow the pattern: `<TYPE>_<LEVEL>[_MODIFIER]([modifier_args...], logger, fmt, ...)`
 *
 * === 1. Standard Logging (`LOG_`, `LOGV_`, `LOGJ_`) ===
 *
 * Pattern: <TYPE>_<LEVEL>(logger, fmt, ...)
 * Description: Logs a message with the specified severity using the chosen formatting engine (`LOG_`, `LOGV_`, `LOGJ_`).
 *
 * === 2. Backtrace Logging (`LOG_BACKTRACE`, `LOGV_BACKTRACE`, `LOGJ_BACKTRACE`) ===
 *
 * Pattern: <TYPE>_BACKTRACE(logger, fmt, ...)
 * Flushes the backtraced logs when a critical message is logged
 * Available Macros: LOG_BACKTRACE, LOGV_BACKTRACE, LOGJ_BACKTRACE
 * Example: LOG_BACKTRACE("Unexpected error code: {}", error_code);
 *
 * === 3. Dynamic Level Logging (`LOG_DYNAMIC`, `LOGV_DYNAMIC`, `LOGJ_DYNAMIC`) ===
 *
 * Pattern: <TYPE>_DYNAMIC(logger, log_level, fmt, ...)
 * Description: Logs a message using a dynamically determined severity level.
 * `log_level` should be of type `quill::LogLevel`.
 * Available Macros: LOG_DYNAMIC, LOGV_DYNAMIC, LOGJ_DYNAMIC
 * Example: EngineLogLevel lvl = some_condition ? EngineLogLevel::Critical : EngineLogLevel::Warning;
 * LOG_DYNAMIC(lvl, "Operation status: {}", status_msg);
 *
 * === 4. Conditional Logging: Rate Limiting (`_LIMIT`) ===
 *
 * Pattern: <TYPE>_<LEVEL>_LIMIT(min_interval, logger, fmt, ...)
 * Description: Limits logging for the *same message at the same source code location* to
 * at most once per `min_interval`.
 * `min_interval`: A `std::chrono::duration` (e.g., `std::chrono::seconds(1)`).
 * Example: LOG_INFO_LIMIT(std::chrono::milliseconds(500), "Processing item {}", item_id);
 *
 * === 5. Conditional Logging: Every N Calls (`_LIMIT_EVERY_N`) ===
 *
 * Pattern: <TYPE>_<LEVEL>_LIMIT_EVERY_N(n_occurrences, logger, fmt, ...)
 * Description: Logs the message only every `n_occurrences` times the macro is executed at that
 * specific source code location. The first call (count 0) is always logged.
 * `n_occurrences`: An integer counter (e.g., `100`).
 * Example: LOG_DEBUG_LIMIT_EVERY_N(100, "Loop iteration {}", i); // Logs on 0, 100, 200...
 *
 * === 6. Tagged Logging (`_TAGS`) ===
 *
 * Pattern: <TYPE>_<LEVEL>_TAGS(logger, tags, fmt, ...)
 * Pattern: <TYPE>_BACKTRACE_TAGS(logger, tags, fmt, ...)
 * Pattern: <TYPE>_DYNAMIC_TAGS(logger, log_level, tags, fmt, ...)
 * Description: Logs a message with associated tags for filtering or structured output.
 * Example:
 * #define NET_TAG "network"
 * LOG_INFO_TAGS(TAGS(NET_TAG), "User '{}' logged in", username);
 * LOG_DYNAMIC_TAGS(lvl, TAGS("status"), "Current status: {}", status_msg);
 *
 * === 7. Runtime Metadata Logging (`LOG_RUNTIME_METADATA`) ===
 *
 * Pattern: LOG_RUNTIME_METADATA(logger, log_level, file, line_number, function, fmt, ...)
 * Description: Logs a message using explicitly provided source location metadata instead of
 * relying on the compiler's `__FILE__`, `__LINE__`, `__FUNCTION__`. Useful for
 * logging libraries wrapping Quill or custom error reporting systems.
 * `file`: C-style string for the filename.
 * `line_number`: C-style string for the line number.
 * `function`: C-style string for the function name.
 * Available Macros: LOG_RUNTIME_METADATA (Only one specific macro for this purpose)
 * Example: LOG_RUNTIME_METADATA(EngineLogLevel::Error, "src/old_module.c", "1234", "legacy_func", "Error in legacy code: {}", err_val);
 */
#define TAGS(...) QUILL_TAGS(__VA_ARGS__)
#pragma region LOG
#ifdef DEBUG
#define LOG_TRACE_L3(fmt, ...) \
    do { QUILL_LOG_TRACE_L3(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L2(fmt, ...) \
    do { QUILL_LOG_TRACE_L2(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L1(fmt, ...) \
    do { QUILL_LOG_TRACE_L1(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_DEBUG(fmt, ...) \
    do { QUILL_LOG_DEBUG(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_INFO(fmt, ...) \
    do { QUILL_LOG_INFO(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_NOTICE(fmt, ...) \
    do { QUILL_LOG_NOTICE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_WARNING(fmt, ...) \
    do { QUILL_LOG_WARNING(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_ERROR(fmt, ...) \
    do { QUILL_LOG_ERROR(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_CRITICAL(fmt, ...) \
    do { QUILL_LOG_CRITICAL(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_BACKTRACE(fmt, ...) \
    do { QUILL_LOG_BACKTRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_DYNAMIC(log_level, fmt, ...) \
    do { QUILL_LOG_DYNAMIC(::LogBackend::getLogger(), MapEngineLogLevelToQuill(log_level), fmt, ##__VA_ARGS__); } while(false)
#else
#define LOG_TRACE_L3(fmt, ...)
#define LOG_TRACE_L2(fmt, ...)
#define LOG_TRACE_L1(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_INFO(fmt, ...)
#define LOG_NOTICE(fmt, ...)
#define LOG_WARNING(fmt, ...)
#define LOG_ERROR(fmt, ...)
#define LOG_CRITICAL(fmt, ...)
#define LOG_BACKTRACE(fmt, ...)
#define LOG_DYNAMIC(log_level, fmt, ...)
#endif
#pragma endregion //LOG
#pragma region LOG_LIMIT
#define LOG_TRACE_L3_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_TRACE_L3_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L2_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_TRACE_L2_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L1_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_TRACE_L1_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_DEBUG_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_DEBUG_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_INFO_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_INFO_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_NOTICE_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_NOTICE_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_WARNING_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_WARNING_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_ERROR_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_ERROR_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_CRITICAL_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOG_CRITICAL_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOG_LIMIT
#pragma region LOG_LIMIT_EVERY_N
#define LOG_TRACE_L3_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_TRACE_L3_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L2_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_TRACE_L2_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L1_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_TRACE_L1_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_DEBUG_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_DEBUG_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_INFO_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_INFO_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_NOTICE_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_NOTICE_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_WARNING_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_WARNING_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_ERROR_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_ERROR_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOG_CRITICAL_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOG_CRITICAL_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOG_LIMIT_EVERY_N
#pragma region LOG_TAGS
#define LOG_TRACE_L3_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_TRACE_L3_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L2_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_TRACE_L2_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_TRACE_L1_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_TRACE_L1_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_DEBUG_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_DEBUG_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_INFO_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_INFO_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_NOTICE_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_NOTICE_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_WARNING_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_WARNING_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_ERROR_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_ERROR_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_CRITICAL_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_CRITICAL_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_BACKTRACE_TAGS(tags, fmt, ...) \
    // do { QUILL_LOG_BACKTRACE_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOG_DYNAMIC_TAGS(log_level, tags, fmt, ...) \
    // do { QUILL_LOG_DYNAMIC_TAGS(::LogBackend::getLogger(), MapEngineLogLevelToQuill(log_level), tags, fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOG_TAGS
#pragma region LOGV
#define LOGV_TRACE_L3(fmt, ...) \
    // do { QUILL_LOGV_TRACE_L3(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L2(fmt, ...) \
    // do { QUILL_LOGV_TRACE_L2(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L1(fmt, ...) \
    // do { QUILL_LOGV_TRACE_L1(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_DEBUG(fmt, ...) \
    // do { QUILL_LOGV_DEBUG(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_INFO(fmt, ...) \
    // do { QUILL_LOGV_INFO(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_NOTICE(fmt, ...) \
    // do { QUILL_LOGV_NOTICE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_WARNING(fmt, ...) \
    // do { QUILL_LOGV_WARNING(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_ERROR(fmt, ...) \
    // do { QUILL_LOGV_ERROR(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_CRITICAL(fmt, ...) \
    // do { QUILL_LOGV_CRITICAL(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_BACKTRACE(fmt, ...) \
    // do { QUILL_LOGV_BACKTRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_DYNAMIC(log_level, fmt, ...) \
    // do {QUILL_LOGV_DYNAMIC(::LogBackend::getLogger(), MapEngineLogLevelToQuill(log_level), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGV
#pragma region LOGV_LIMIT
#define LOGV_TRACE_L3_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L3_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L2_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L2_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L1_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L1_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_DEBUG_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_DEBUG_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_INFO_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_INFO_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_NOTICE_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_NOTICE_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_WARNING_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_WARNING_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_ERROR_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_ERROR_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_CRITICAL_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGV_CRITICAL_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGV_LIMIT
#pragma region LOGV_LIMIT_EVERY_N
#define LOGV_TRACE_L3_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L3_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L2_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L2_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L1_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L1_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_DEBUG_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_DEBUG_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_INFO_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_INFO_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_NOTICE_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_NOTICE_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_WARNING_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_WARNING_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_ERROR_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_ERROR_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGV_CRITICAL_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGV_CRITICAL_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGV_LIMIT_EVERY_N
#pragma region LOGV_TAGS
#define LOGV_TRACE_L3_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L3_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L2_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L2_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_TRACE_L1_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_TRACE_L1_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_DEBUG_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_DEBUG_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_INFO_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_INFO_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_NOTICE_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_NOTICE_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_WARNING_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_WARNING_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_ERROR_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_ERROR_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGV_CRITICAL_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGV_CRITICAL_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGV_TAGS
#pragma region LOGJ
#define LOGJ_TRACE_L3(fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L3(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L2(fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L2(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L1(fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L1(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_DEBUG(fmt, ...) \
    // do { QUILL_LOGJ_DEBUG(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_INFO(fmt, ...) \
    // do { QUILL_LOGJ_INFO(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_NOTICE(fmt, ...) \
    // do { QUILL_LOGJ_NOTICE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_WARNING(fmt, ...) \
    // do { QUILL_LOGJ_WARNING(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_ERROR(fmt, ...) \
    // do { QUILL_LOGJ_ERROR(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_CRITICAL(fmt, ...) \
    // do { QUILL_LOGJ_CRITICAL(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_BACKTRACE(fmt, ...) \
    // do { QUILL_LOGJ_BACKTRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_DYNAMIC(log_level, fmt, ...) \
    // do { QUILL_LOGJ_DYNAMIC(::LogBackend::getLogger(), MapEngineLogLevelToQuill(log_level), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGJ
#pragma region LOGJ_LIMIT
#define LOGJ_TRACE_L3_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L3_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L2_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L2_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L1_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L1_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_DEBUG_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_DEBUG_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_INFO_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_INFO_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_NOTICE_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_NOTICE_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_WARNING_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_WARNING_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_ERROR_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_ERROR_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_CRITICAL_LIMIT(min_interval, fmt, ...) \
    // do { QUILL_LOGJ_CRITICAL_LIMIT(min_interval, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGJ_LIMIT
#pragma region LOGJ_LIMIT_EVERY_N
#define LOGJ_TRACE_L3_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L3_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L2_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L2_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L1_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L1_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_DEBUG_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_DEBUG_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_INFO_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_INFO_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_NOTICE_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_NOTICE_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_WARNING_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_WARNING_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_ERROR_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_ERROR_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_CRITICAL_LIMIT_EVERY_N(n_occurrences, fmt, ...) \
    // do { QUILL_LOGJ_CRITICAL_LIMIT_EVERY_N(n_occurrences, ::LogBackend::getLogger(), fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGJ_LIMIT_EVERY_N
#pragma region LOGJ_TAGS
#define LOGJ_TRACE_L3_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L3_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L2_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L2_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_TRACE_L1_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_TRACE_L1_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_DEBUG_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_DEBUG_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_INFO_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_INFO_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_NOTICE_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_NOTICE_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_WARNING_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_WARNING_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_ERROR_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_ERROR_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#define LOGJ_CRITICAL_TAGS(tags, fmt, ...) \
    // do { QUILL_LOGJ_CRITICAL_TAGS(::LogBackend::getLogger(), tags, fmt, ##__VA_ARGS__); } while(false)
#pragma endregion //LOGJ_TAGS
#define LOG_RUNTIME_METADATA(log_level, file, line_number, function, fmt, ...) \
    // do { QUILL_LOG_RUNTIME_METADATA(::LogBackend::getLogger(), MapEngineLogLevelToQuill(log_level), file, line_number, function, fmt, ##__VA_ARGS__); } while(false)

#endif // ASSET_COMPILER_BUILD
