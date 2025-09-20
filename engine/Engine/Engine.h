/******************************************************************************/
/*!
\file   Engine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the Engine class.
The Engine class is responsible for initializing the game engine, running the game loop, and shutting down the engine.
It includes methods for initializing the engine, running the game loop, and shutting down the engine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "game.h"
#include "ResourceManager.h"
#include "Console.h"
#include "Performance.h"
#include "CustomViewport.h"
#include "AudioManager.h"
#include "AssetBrowser.h"
#include "PrefabWindow.h"
#include "Hierarchy.h"
#include "Popup.h"
#include "Editor.h"
#include "context.h"

#include "BehaviourTree.h"

namespace editor
{
    class ImGuiContext;
}

class Renderer;

namespace AssetLoading
{
    class AssetSystem;
}

/*****************************************************************//*!
\class Engine
\brief
    The manager of the program.
*//******************************************************************/
class Engine {
public:
    using clock = std::chrono::steady_clock;  // More reliable than system_clock
    using duration = std::chrono::nanoseconds; // Higher precision
    using time_point = std::chrono::time_point<clock>;

    Engine();
        BehaviorTree bt;

    ~Engine();
    // Prevent copying
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // Allow moving
    Engine(Engine&&) = default;
    Engine& operator=(Engine&&) = default;

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
    double fps {};
    duration m_targetFrameTime{};
    time_point m_lastFrameTime{};
    
    // Spin-wait threshold - sleep for longer waits, spin for shorter ones
    static constexpr duration SPIN_THRESHOLD = std::chrono::microseconds(500);
};
