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

#else // Full spdlog logging for MagicEngine

#include "log_backend.h"
#include <spdlog/spdlog.h>

// Compile-time stripping: When SPDLOG_ACTIVE_LEVEL is set (e.g. to SPDLOG_LEVEL_WARN in Release),
// SPDLOG_LOGGER_TRACE, SPDLOG_LOGGER_DEBUG, and SPDLOG_LOGGER_INFO expand to ((void)0).

// All three quill trace levels collapse to spdlog's single trace level.
#define LOG_TRACE_L3(fmt, ...) SPDLOG_LOGGER_TRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_TRACE_L2(fmt, ...) SPDLOG_LOGGER_TRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_TRACE_L1(fmt, ...) SPDLOG_LOGGER_TRACE(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)    SPDLOG_LOGGER_DEBUG(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)     SPDLOG_LOGGER_INFO(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_NOTICE(fmt, ...)   SPDLOG_LOGGER_INFO(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_WARNING(fmt, ...)  SPDLOG_LOGGER_WARN(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)    SPDLOG_LOGGER_ERROR(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) SPDLOG_LOGGER_CRITICAL(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)
#define LOG_BACKTRACE(fmt, ...) ((void)0)
#define LOG_DYNAMIC(log_level, fmt, ...) ((void)0)
#define LOG_RUNTIME_METADATA(log_level, file, line_number, function, fmt, ...) \
    SPDLOG_LOGGER_CRITICAL(::LogBackend::getLogger(), fmt, ##__VA_ARGS__)

#endif // ASSET_COMPILER_BUILD
