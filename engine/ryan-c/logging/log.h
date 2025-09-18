#pragma once

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
 * The original docs can be found here. https://quillcpp.readthedocs.io/en/latest/logging_macros.html
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

#define COMPILE_OUT_LOG_MACRO(...) (void)0

#pragma region LOG
#define LOG_TRACE_L3(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L2(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L1(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DEBUG(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_INFO(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_NOTICE(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_WARNING(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_ERROR(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_CRITICAL(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_BACKTRACE(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DYNAMIC(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOG_LIMIT
#define LOG_TRACE_L3_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L2_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L1_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DEBUG_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_INFO_LIMIT(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_NOTICE_LIMIT(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_WARNING_LIMIT(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_ERROR_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_CRITICAL_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOG_LIMIT_EVERY_N
#define LOG_TRACE_L3_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L2_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L1_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DEBUG_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_INFO_LIMIT_EVERY_N(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_NOTICE_LIMIT_EVERY_N(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_WARNING_LIMIT_EVERY_N(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_ERROR_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_CRITICAL_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOG_TAGS
#define TAGS(...)
#define LOG_TRACE_L3_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L2_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_TRACE_L1_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DEBUG_TAGS(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_INFO_TAGS(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_NOTICE_TAGS(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_WARNING_TAGS(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_ERROR_TAGS(...)    COMPILE_OUT_log_macro(__VA_ARGS__)
#define LOG_CRITICAL_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_BACKTRACE_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOG_DYNAMIC_TAGS(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGV
#define LOGV_TRACE_L3(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L2(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L1(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_DEBUG(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_INFO(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_NOTICE(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_WARNING(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_ERROR(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_CRITICAL(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_BACKTRACE(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_DYNAMIC(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGV_LIMIT
#define LOGV_TRACE_L3_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L2_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L1_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_DEBUG_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_INFO_LIMIT(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_NOTICE_LIMIT(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_WARNING_LIMIT(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_ERROR_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_CRITICAL_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGV_LIMIT_EVERY_N
#define LOGV_TRACE_L3_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L2_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L1_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_DEBUG_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_INFO_LIMIT_EVERY_N(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_NOTICE_LIMIT_EVERY_N(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_WARNING_LIMIT_EVERY_N(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_ERROR_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_CRITICAL_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGV_TAGS
#define LOGV_TRACE_L3_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L2_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_TRACE_L1_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_DEBUG_TAGS(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_INFO_TAGS(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_NOTICE_TAGS(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_WARNING_TAGS(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_ERROR_TAGS(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGV_CRITICAL_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGJ
#define LOGJ_TRACE_L3(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L2(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L1(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_DEBUG(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_INFO(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_NOTICE(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_WARNING(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_ERROR(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_CRITICAL(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_BACKTRACE(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_DYNAMIC(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGJ_LIMIT
#define LOGJ_TRACE_L3_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L2_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L1_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_DEBUG_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_INFO_LIMIT(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_NOTICE_LIMIT(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_WARNING_LIMIT(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_ERROR_LIMIT(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_CRITICAL_LIMIT(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGJ_LIMIT_EVERY_N
#define LOGJ_TRACE_L3_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L2_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L1_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_DEBUG_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_INFO_LIMIT_EVERY_N(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_NOTICE_LIMIT_EVERY_N(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_WARNING_LIMIT_EVERY_N(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_ERROR_LIMIT_EVERY_N(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_CRITICAL_LIMIT_EVERY_N(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#pragma region LOGJ_TAGS
#define LOGJ_TRACE_L3_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L2_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_TRACE_L1_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_DEBUG_TAGS(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_INFO_TAGS(...)     COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_NOTICE_TAGS(...)   COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_WARNING_TAGS(...)  COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_ERROR_TAGS(...)    COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#define LOGJ_CRITICAL_TAGS(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)
#pragma endregion

#define LOG_RUNTIME_METADATA(...) COMPILE_OUT_LOG_MACRO(__VA_ARGS__)