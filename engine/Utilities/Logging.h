#pragma once
#include "Singleton.h"

enum LogLevel
{
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARNING,
    LEVEL_ERROR,
    LEVEL_FATAL
};

#define CONSOLE_LOG(logLevel) ST<::internal::LoggedMessagesBuffer>::Get()->Log(logLevel)
#define CONSOLE_LOG_DIRECT(msg) ST<::internal::LoggedMessagesBuffer>::Get()->Log(msg)
#define CONSOLE_LOG_EXPLICIT(msg, logLevel) ST<::internal::LoggedMessagesBuffer>::Get()->Log(msg, logLevel)
#define CONSOLE_LOG_UNIMPLEMENTED() ST<::internal::LoggedMessagesBuffer>::Get()->Log(LogLevel::LEVEL_FATAL).SetUnimplementedFlag()

namespace internal {

    class LoggedMessagesBuffer
    {
    private:
        friend ST<LoggedMessagesBuffer>;
        LoggedMessagesBuffer();

    public:
        /*!
        \class Logger
        \brief
        A nested class within Console that handles the formatting and logging of messages
        at a specified log level. This class allows chaining of log message additions
        through the overloaded << operator.

        \param level The log level associated with the message being logged.
        \param console Reference to the Console instance for logging operations.
        */
        class Logger
        {
        public:
            /*!
            \brief Constructor for Logger class.
            \param level The log level for this Logger instance.
            \param console Reference to the Console instance for logging messages.
            */
            Logger(LogLevel level, LoggedMessagesBuffer* buffer);

            /*!
            \brief Overloaded operator for chaining log messages.
            \param value The value to be logged.
            \return Reference to the Logger instance for further chaining.
            */
            template<typename T>
            Logger& operator<<(const T& value);

            /*!
            \brief Destructor for Logger class, flushing the log message to the console.
            */
            ~Logger();

            Logger& SetUnimplementedFlag();

        private:
            /*!
            \brief Flushes the accumulated log message to the logger.
            */
            void Flush();

            LogLevel logLevel;         ///< The log level associated with this Logger instance.
            LoggedMessagesBuffer* buffer;   ///< Reference to the Buffer instance for logging. If null, logging does nothing.
            std::ostringstream messageBuffer; ///< Buffer to accumulate log message content.
            //! Whether this log should throw NotImplementedException when destroyed.
            bool unimplementedLogFlag;
        };

        struct LogEntry
        {
            std::string message; ///< The log message.
            LogLevel level;      ///< The log level of the message.
        };

    public:
        /*!
        \brief Logs a message at the specified log level.
        \param level The log level for the message (e.g., LEVEL_INFO).
        \return A Logger instance for chaining log messages.
        */
        Logger Log(LogLevel level);

        /*!
        \brief Clears all log entries stored in the logger.
        */
        void ClearLog();

        /*!
        \brief Sets the minimum log level of the console.
        \param level The log level of the console.
        */
        void SetLogLevel(LogLevel level);

        /*!
        \brief Logs a message with the specified log level.
        \param message The message to log.
        \param level The log level of the message (default is LEVEL_INFO).
        */
        void Log(std::string_view message, LogLevel level = LEVEL_INFO);

        /*!
        \brief Flushes log entries to a specified file.
        \param filename The name of the file to which logs will be written.
        */
        void FlushLogToFile(const std::string& filename);

        const std::vector<LogEntry>& GetLogs() const;

    private:
        /*!
        \brief Adds a log message with an optional log level.
        \param message The message to log.
        \param level The log level of the message (default is LEVEL_INFO).
        */
        void AddLog(std::string_view message, LogLevel level = LEVEL_INFO);

        /*!
        \brief Creates a formatted log message based on the log level and message.
        \param level The log level associated with the message.
        \param message The message to format.
        \return A formatted string representing the log message.
        */
        std::string CreateFormattedMessage(LogLevel level, std::string_view message);

    private:

        std::vector<LogEntry> log;      ///< Vector to store log entries.
        size_t log_count;          ///< Counter for the number of log entries.
        LogLevel logLevel; ///< Default log level for the logger.

        static constexpr size_t MAX_LOG_ENTRIES = 1000; ///< Maximum number of log entries to store.

    };



    template <typename T>
    LoggedMessagesBuffer::Logger& LoggedMessagesBuffer::Logger::operator<<(const T& value) {
        if (buffer)
            messageBuffer << value;
        return *this;
    }

}

