/******************************************************************************/
/*!
\file   Console.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the Console class, which is responsible
for managing logging and console output for the application. It includes
methods for handling log messages, formatting them, and managing their display
and storage. The Console class supports different log levels and can flush logs
to a file. It also includes an unhandled exception handler and a method for
dumping stack traces.
The Console class is implemented as a singleton to ensure that only one instance
exists throughout the application. The class also includes a nested Logger class
template for logging messages.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "GUICollection.h"
#pragma comment(lib, "dbghelp.lib")

// Shortform for ST<Console>::Get()->Log(logLevel)
#define CONSOLE_LOG(logLevel) ST<Console>::Get()->Log(logLevel)
// Shortform for ST<Console>::Get()->Log(msg, LogLevel::LEVEL_INFO)
#define CONSOLE_LOG_DIRECT(msg) ST<Console>::Get()->Log(msg)
// Shortform for ST<Console>::Get()->Log(msg, logLevel)
#define CONSOLE_LOG_EXPLICIT(msg, logLevel) ST<Console>::Get()->Log(msg, logLevel)

enum LogLevel
{
    LEVEL_DEBUG,
    LEVEL_INFO,
    LEVEL_WARNING,
    LEVEL_ERROR,
    LEVEL_FATAL
};


class Console : public gui::Window
{
public:
    friend class ST<Console>;

    /*!
    \class Logger
    \brief
    A nested class within Console that handles the formatting and logging of messages
    at a specified log level. This class allows chaining of log message additions
    through the overloaded << operator.

    \param level The log level associated with the message being logged.
    \param console Reference to the Console instance for logging operations.
    */
    class Logger {
    public:
        /*!
        \brief Constructor for Logger class.
        \param level The log level for this Logger instance.
        \param console Reference to the Console instance for logging messages.
        */
        Logger(LogLevel level, Console* console) : logLevel(level), console(console) {}

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

    private:
        /*!
        \brief Flushes the accumulated log message to the console.
        */
        void Flush();

        LogLevel logLevel;         ///< The log level associated with this Logger instance.
        Console* console;          ///< Reference to the Console instance for logging. If null, logging does nothing.
        std::ostringstream messageBuffer; ///< Buffer to accumulate log message content.
    };
    /*!
    \brief Logs a message at the specified log level.
    \param level The log level for the message (e.g., LEVEL_INFO).
    \return A Logger instance for chaining log messages.
    */
    Logger Log(LogLevel level)
    {
        return { level, level >= logLevel ? this : nullptr };
    }

    /*!
   \brief Clears all log entries stored in the console.
   */
    void ClearLog();


#ifdef IMGUI_ENABLED
    /*!
    \brief Handles command input from the user in the console.
    \param command_line The command string input by the user.
    */
    void HandleCommand(const char* command_line);

    /*!
    \brief Prints available commands to the console.
    */
    void PrintAvailableCommands();
#endif
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
    \brief Dumps all log entries to a specified file.
    \param filename The name of the file to which logs will be written.
    */
    void DumpToFile(const std::string& filename);

    /*!
    \brief Sets up an unhandled exception handler for the application.
    */
    void SetupCrashHandler();

private:
    /*!
    \brief Private constructor for the Console class, enforcing singleton pattern.
    */
    Console();

    Console(const Console&) = delete; ///< Disable copy constructor.
    Console& operator=(const Console&) = delete; ///< Disable copy assignment operator.

    /*!
    \brief Adds a log message with an optional log level.
    \param message The message to log.
    \param level The log level of the message (default is LEVEL_INFO).
    */
    void AddLog(std::string_view message, LogLevel level = LEVEL_INFO);

    /*!
    \brief Flushes log entries to a specified file.
    \param filename The name of the file to which logs will be written.
    */
    void FlushLogToFile(const std::string& filename);

    /*!
    \brief Draws the console log window and displays log entries using ImGui.
    */
    void DrawContents() final;

    /*****************************************************************//*!
    \brief
        Watches for window open state changes.
    *//******************************************************************/
    void OnOpenStateChanged() override;

#ifdef IMGUI_ENABLED
    /*!
    \brief Filters input for ImGui input text.
    \param data Pointer to the ImGui input text callback data.
    \return 1 if the character input should be added. 0 otherwise.
    */
    static int InputTextFilter(gui::types::InputTextCallbackData* data);
#endif

    /*!
    \brief Creates a formatted log message based on the log level and message.
    \param level The log level associated with the message.
    \param message The message to format.
    \return A formatted string representing the log message.
    */
    std::string CreateFormattedMessage(LogLevel level, std::string_view message);

    /*!
    \brief Handles unhandled exceptions by invoking a custom handler.
    \param exceptionInfo Pointer to exception information.
    \return A LONG indicating the result of the handler.
    */
    static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* exceptionInfo);

    /*!
    \brief Dumps the current stack trace to a file.
    \param file The output file stream where the stack trace will be written.
    */
    void DumpStackTrace(std::ofstream& file);

    struct LogEntry {
        std::string message; ///< The log message.
        LogLevel level;      ///< The log level of the message.
    };

    std::vector<LogEntry> log;      ///< Vector to store log entries.
    size_t log_count{ 0 };          ///< Counter for the number of log entries.
    gui::TextBoxWithBuffer<256> inputTextBox; ///< Text box for command input.
    bool scrollToBottom = true;      ///< Flag indicating if the log should scroll to the bottom.
    bool toggleScroll = false;       ///< Flag to toggle scrolling to the bottom.
    LogLevel logLevel = LEVEL_DEBUG; ///< Default log level for the console.

    bool showDebug = false;  ///< Flag to show/hide debug messages.
    bool showInfo = true;    ///< Flag to show/hide info messages.
    bool showWarning = true;  ///< Flag to show/hide warning messages.
    bool showError = true;    ///< Flag to show/hide error messages.
    bool showFatal = true;    ///< Flag to show/hide fatal messages.

    const size_t MAX_LOG_ENTRIES = 1000; ///< Maximum number of log entries to store.

    bool stealFocus = false; ///< Flag on whether to steal keyboard focus this frame.

    //! Signature of functions that are executed by a user command
    using UserCmdFuncSig = void(*)(Console& console, const std::vector<std::string>& tokens);
    std::unordered_map<std::string, UserCmdFuncSig> cmdMap; ///< Map of command string to function that executes that command
};

template <typename T>
Console::Logger& Console::Logger::operator<<(const T& value) {
    if (console)
        messageBuffer << value;
    return *this;
}

