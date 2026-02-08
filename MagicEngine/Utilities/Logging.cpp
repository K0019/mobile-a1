/******************************************************************************/
/*!
\file   Logging.cpp
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   09/23/2025

\author Ryan Cheong (90%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Kendrick Sim Hean Guan (10%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Saves logging messages from the program into a buffer and file.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Utilities/Logging.h"

#ifndef GLFW
#include <android/log.h>
#define LOG_TAG "MagicEngine"
#endif

namespace internal {

	LoggedMessagesBuffer::Logger::Logger(LogLevel level, LoggedMessagesBuffer* buffer)
		: logLevel{ level }
		, buffer{ buffer }
		, unimplementedLogFlag{}
	{
	}

	LoggedMessagesBuffer::Logger::~Logger() {
		Flush();
		if (unimplementedLogFlag)
			assert(false); // Code execution reached an unimplemented function! Please proceed to bug the person responsible for that code.
	}

	LoggedMessagesBuffer::Logger& LoggedMessagesBuffer::Logger::SetUnimplementedFlag()
	{
		unimplementedLogFlag = true;
		messageBuffer << "Code execution reached an unimplemented function! -- ";
		return *this;
	}

	void LoggedMessagesBuffer::Logger::Flush()
	{
		if (buffer && !messageBuffer.str().empty())
		{
			buffer->Log(messageBuffer.str(), logLevel);
			messageBuffer.clear();
		}
	}

	LoggedMessagesBuffer::LoggedMessagesBuffer()
		: log_count{}
#ifdef NDEBUG
		, logLevel{ LEVEL_WARNING }
#else
		, logLevel{ LEVEL_DEBUG }
#endif
	{
		log.reserve(MAX_LOG_ENTRIES);
	}

	LoggedMessagesBuffer::Logger LoggedMessagesBuffer::Log(LogLevel level)
	{
		return { level, level >= logLevel ? this : nullptr };
	}

	void LoggedMessagesBuffer::ClearLog()
	{
		log.clear();
		log_count = 0;
	}

	void LoggedMessagesBuffer::SetLogLevel(LogLevel level)
	{
		logLevel = level;
	}

	void LoggedMessagesBuffer::Log(std::string_view message, LogLevel level)
	{
		if (level >= logLevel)
		{
			AddLog(message, level);  // AddLog will handle converting string_view to string
		}
	}

	void LoggedMessagesBuffer::FlushLogToFile(const std::string& filename)
	{
		std::ofstream file(filename, std::ios::app);
		if (!file.is_open())
		{
			Log(LEVEL_ERROR) << "Failed to open log file: " << filename;
			return;
		}

		bool isAtMaxCapacity{ log_count >= MAX_LOG_ENTRIES };
		for (const auto& entry : log)
		{
			file << CreateFormattedMessage(entry.level, entry.message) << std::endl;
		}

		file.close();
		ClearLog();

		if (isAtMaxCapacity)
			Log(LEVEL_WARNING) << "Log at Maximum Capacity, flushing log to " << filename;
		else
			Log(LEVEL_INFO) << "Log flushed to file: " << filename;
	}

	const std::vector<LoggedMessagesBuffer::LogEntry>& LoggedMessagesBuffer::GetLogs() const
	{
		return log;
	}

	void LoggedMessagesBuffer::AddLog(std::string_view message, LogLevel level)
	{
		log.push_back({ std::string(message), level });
#if defined(_DEBUG) && defined(GLFW)
		std::cout << CreateFormattedMessage(level, message) << '\n';
#elif !defined(GLFW)
		android_LogPriority priority{};
		switch (level)
		{
		case LEVEL_DEBUG: priority = ANDROID_LOG_DEBUG; break;
		case LEVEL_INFO: priority = ANDROID_LOG_INFO; break;
		case LEVEL_WARNING: priority = ANDROID_LOG_WARN; break;
		case LEVEL_ERROR: priority = ANDROID_LOG_ERROR; break;
		case LEVEL_FATAL: priority = ANDROID_LOG_FATAL; break;
		}
#ifndef NDEBUG
		__android_log_print(priority, LOG_TAG, "%s", message.data());
#else
		if (level >= LEVEL_WARNING)
			__android_log_print(priority, LOG_TAG, "%s", message.data());
#endif
#endif
		if (log_count < MAX_LOG_ENTRIES)
			log_count++;
		else
		{
#ifdef GLFW
			FlushLogToFile("console_log.txt");
#endif
			ClearLog();
		}
	}

	std::string LoggedMessagesBuffer::CreateFormattedMessage(LogLevel level, std::string_view message)
	{
		auto now = std::chrono::system_clock::now();
		auto now_time_t = std::chrono::system_clock::to_time_t(now);
		auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

		std::tm tm_time;
		// wtf is this lack of standardization
#ifdef WIN32
		localtime_s(&tm_time, &now_time_t);
#else
		localtime_r(&now_time_t, &tm_time);
#endif

		std::ostringstream oss;
		oss << std::put_time(&tm_time, "%Y-%m-%d %H:%M:%S");
		oss << '.' << std::setfill('0') << std::setw(3) << now_ms.count();

		switch (level)
		{
		case LEVEL_DEBUG:   oss << " [DEBUG] "; break;
		case LEVEL_INFO:    oss << " [INFO] "; break;
		case LEVEL_WARNING: oss << " [WARNING] "; break;
		case LEVEL_ERROR:   oss << " [ERROR] "; break;
		case LEVEL_FATAL:   oss << " [FATAL] "; break;
		}

		oss << message;
		return oss.str();
	}

	template <>
	LoggedMessagesBuffer::Logger& LoggedMessagesBuffer::Logger::operator<<(const Vec3& value) {
		if (buffer)
			messageBuffer << '(' << value.x << ", " << value.y << ", " << value.z << ')';
		return *this;
	}

}
