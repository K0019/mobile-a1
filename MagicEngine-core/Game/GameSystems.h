/******************************************************************************/
/*!
\file   GameSystems.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for state classes that manage which systems are
  loaded into ECS.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Engine/StateMachine.h"

#pragma region Interface

//! Represents the current state of the game
enum class GAMESTATE : int
{
	NONE,
	EDITOR,
	IN_GAME,
	PAUSE
};

/*****************************************************************//*!
\class GameStateBase
\brief
	The base class for game states. Removes all ecs systems upon exit.
*//******************************************************************/
class GameStateBase : public sm::SimpleState
{
public:
	/*****************************************************************//*!
	\brief
		Removes all ecs systems.
	*//******************************************************************/
	void OnExit() final;
};

/*****************************************************************//*!
\class GameState_Common
\brief
	The base class for game states that share common ecs systems such
	as render and text.
*//******************************************************************/
class GameState_Common : public GameStateBase
{
public:
	/*****************************************************************//*!
	\brief
		Inserts ecs systems that in theory should be common across most
		game states, such as render and text.
	*//******************************************************************/
	virtual void OnEnter() override;
};

/*****************************************************************//*!
\class GameState_Editor
\brief
	The game state which loads ecs systems required when in editor mode.
*//******************************************************************/
class GameState_Editor : public GameState_Common
{
public:
	/*****************************************************************//*!
	\brief
		Loads ecs systems required in editor mode.
	*//******************************************************************/
	void OnEnter() final;

};

/*****************************************************************//*!
\class GameState_Game
\brief
	The game state which loads ecs systems required when in play mode.
*//******************************************************************/
class GameState_Game : public GameState_Common
{
public:
	/*****************************************************************//*!
	\brief
		Loads ecs systems required in play mode.
	*//******************************************************************/
	void OnEnter() final;
};

/*****************************************************************//*!
\class GameState_Pause
\brief
	The game state which loads ecs systems required when in pause mode.
*//******************************************************************/
class GameState_Pause : public GameState_Common
{
public:
	/*****************************************************************//*!
	\brief
		Loads ecs systems required in play mode.
	*//******************************************************************/
	void OnEnter() final;
};


/*****************************************************************//*!
\class GameSystemsManager
\brief
	A state machine with an interface specifically for managing game states.
*//******************************************************************/
class GameSystemsManager : private sm::SimpleStateMachine
{
private:
	GameSystemsManager();
	friend ST<GameSystemsManager>;

public:
	/*****************************************************************//*!
	\brief
		Initializes the state machine.
	\param initialState
		The initial game state.
	*//******************************************************************/
	void Init(GAMESTATE initalState);

	/*****************************************************************//*!
	\brief
		Gets the current state of the game.
	\return
		The current state of the game.
	*//******************************************************************/
	GAMESTATE GetState() const;

	/*****************************************************************//*!
	\brief
		Updates the game state to the next state, replacing ECS systems
		with those in the new state.
	*//******************************************************************/
	void UpdateState();

	/*****************************************************************//*!
	\brief
		Resets all loaded ECS systems in the current state.
	*//******************************************************************/
	void ResetState();

	/*****************************************************************//*!
	\brief
		Shuts down the state machine.
	*//******************************************************************/
	void Exit();

private:
	/*****************************************************************//*!
	\brief
		Switches game state to the specified class type.
	\tparam GameStateType
		The type of the game state.
	*//******************************************************************/
	template <typename GameStateType>
	void SwitchToState();

	/*****************************************************************//*!
	\brief
		Updates the state machine managing the game state to the specified state.
	\param newState
		The new game state.
	*//******************************************************************/
	void UpdateState(GAMESTATE newState);

private:
	/*****************************************************************//*!
	\brief
		Toggles the gamemode between editor mode and game mode.
	*//******************************************************************/
	static void OnTogglePlayMode();

	/*****************************************************************//*!
	\brief
		Toggles the gamemode between game mode and pause mode.
	*//******************************************************************/
	static void OnTogglePauseMode();

private:
	GAMESTATE state, nextState;
	bool flaggedForReset;
};

#pragma endregion // Interface

#pragma region Definition

template<typename GameStateType>
void GameSystemsManager::SwitchToState()
{
	SimpleStateMachine::SwitchToState(new GameStateType{});
}

#pragma endregion Definition
