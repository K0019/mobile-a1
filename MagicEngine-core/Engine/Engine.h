/******************************************************************************/
/*!
\file   MagicEngine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the MagicEngine class.
The MagicEngine class is responsible for initializing the game engine, running the game loop, and shutting down the engine.
It includes methods for initializing the engine, running the game loop, and shutting down the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Game/game.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Console.h"
#include "Editor/Performance.h"
#include "Graphics/CustomViewport.h"
#include "Managers/AudioManager.h"
#include "Editor/AssetBrowser.h"
#include "Editor/PrefabWindow.h"
#include "Editor/Hierarchy.h"
#include "Editor/Popup.h"
#include "Editor/Editor.h"
#include "engine/engine.h"

namespace editor
{
    class ImGuiContext;
}

class Renderer;

namespace Resource
{
    class ResourceManager;
}

/*****************************************************************//*!
\class MagicEngine
\brief
    The manager of the program.
*//******************************************************************/
class MagicEngine {
public:
    using clock = std::chrono::steady_clock;  // More reliable than system_clock
    using duration = std::chrono::nanoseconds; // Higher precision
    using time_point = std::chrono::time_point<clock>;

    MagicEngine();
    ~MagicEngine();
    // Prevent copying
    MagicEngine(const MagicEngine&) = delete;
    MagicEngine& operator=(const MagicEngine&) = delete;

    // Allow moving
    MagicEngine(MagicEngine&&) = default;
    MagicEngine& operator=(MagicEngine&&) = default;

    void setFPS(double _fps);
    void wait();

    /*****************************************************************//*!
    \brief
        Marks the program to shutdown.
    *//******************************************************************/
    void MarkToShutdown();

    /*****************************************************************//*!
    \brief
        Gets whether the program is marked to shutdown.
    *//******************************************************************/
    bool IsShuttingDown() const;

    void init();
    void shutdown();
    void run();

private:
    double fps{};
    duration m_targetFrameTime{};
    time_point m_lastFrameTime{};

    // Spin-wait threshold - sleep for longer waits, spin for shorter ones
    static constexpr duration SPIN_THRESHOLD = std::chrono::microseconds(500);
};
