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
#include "Editor/Containers/GUIAsECS.h"

namespace editor {

    class Console : public WindowBase<Console>
    {
    public:
        Console();

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

    private:
        /*!
        \brief Draws the console log window and displays log entries using ImGui.
        */
        void DrawWindow() final;

        /*****************************************************************//*!
        \brief
            Watches for window open state changes.
        *//******************************************************************/
        void UserOnOpenStateChanged() override;

#ifdef IMGUI_ENABLED
        /*!
        \brief Filters input for ImGui input text.
        \param data Pointer to the ImGui input text callback data.
        \return 1 if the character input should be added. 0 otherwise.
        */
        static int InputTextFilter(gui::types::InputTextCallbackData* data);
#endif

        gui::TextBoxWithBuffer<256> inputTextBox; ///< Text box for command input.
        bool scrollToBottom = true;      ///< Flag indicating if the log should scroll to the bottom.
        bool toggleScroll = false;       ///< Flag to toggle scrolling to the bottom.

        bool showDebug = false;  ///< Flag to show/hide debug messages.
        bool showInfo = true;    ///< Flag to show/hide info messages.
        bool showWarning = true;  ///< Flag to show/hide warning messages.
        bool showError = true;    ///< Flag to show/hide error messages.
        bool showFatal = true;    ///< Flag to show/hide fatal messages.

        bool stealFocus = false; ///< Flag on whether to steal keyboard focus this frame.

        //! Signature of functions that are executed by a user command
        using UserCmdFuncSig = void(*)(Console& console, const std::vector<std::string>& tokens);
        std::unordered_map<std::string, UserCmdFuncSig> cmdMap; ///< Map of command string to function that executes that command
    };

}
