/******************************************************************************/
/*!
\file   Game.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
  This is an interface file for the game scene.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "GameSystems.h"

//! Represents the current state of the game
enum class GAMESTATE : int
{
    NONE,
    EDITOR,
    IN_GAME,
    PAUSE
};

// Game holds all game-related state and functionality.
class Game
{
public:

    /*****************************************************************//*!
    \brief
        Constructor.
    *//******************************************************************/
    Game();

    /*****************************************************************//*!
    \brief
        Destructor.
    *//******************************************************************/
    ~Game();

    /*****************************************************************//*!
    \brief
        Initializes the game state (load all Assets/Shaders/textures/levels)
    \param width
        The internal render resolution width.
    \param height
        The internal render resolution height.
    \param firstState
        The initial state of the game.
    *//******************************************************************/
    void Init(unsigned int width, unsigned int height, GAMESTATE firstState);

    /*****************************************************************//*!
    \brief
        Updates systems that depend on user input.
    *//******************************************************************/
    void ProcessInput();

    /*****************************************************************//*!
    \brief
        Updates general systems.
    *//******************************************************************/
    void Update();

    /*****************************************************************//*!
    \brief
        Renders the game scene.
    *//******************************************************************/
    void Render();

    /*****************************************************************//*!
    \brief
        Shuts down the game scene.
    *//******************************************************************/
    void Shutdown();

    /*****************************************************************//*!
    \brief
        Gets the current state of the game.
    \return
        The current state of the game.
    *//******************************************************************/
    GAMESTATE GetState() const;

    /*****************************************************************//*!
    \brief
        Sets the set of systems that are loaded within ECS.
    \param state
        What state to put the ECS systems in.
    *//******************************************************************/
    void SetState(GAMESTATE newState);

    /*****************************************************************//*!
    \brief
        Resets all loaded ECS systems in the current state.
    *//******************************************************************/
    void ResetState();

    /*****************************************************************//*!
    \brief
        Toggles the gamemode between editor mode and game mode.
    *//******************************************************************/
    void TogglePlayMode();

    /*****************************************************************//*!
    \brief
        Toggles the gamemode between game mode and pause mode.
    *//******************************************************************/
    void TogglePauseMode();

private:
    /*****************************************************************//*!
    \brief
        Updates the state machine managing the game state to the next state.
    *//******************************************************************/
    void UpdateState();

    /*****************************************************************//*!
    \brief
        Updates the state machine managing the game state to the specified state.
    \param newState
        The new game state.
    *//******************************************************************/
    void UpdateState(GAMESTATE newState);

    /*****************************************************************//*!
    \brief
        Executes a function that runs systems within a profile.
    \param profileName
        The name of the profile.
    \param executeSystemsFunc
        The function that runs the systems.
    *//******************************************************************/
    void UpdateSystemsGroup(const std::string& profileName, void(*executeSystemsFunc)());

    /*****************************************************************//*!
    \brief
        TEMPORARY: Loads the pause scene if we're in game and we've lost focused.
    *//******************************************************************/
    static void OnFocusCallback(bool isFocused);

    GameStateManager gameStateManager;

    GAMESTATE state, nextState;
    unsigned int Width, Height;

    bool flaggedForReset;
};