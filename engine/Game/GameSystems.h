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

#pragma region Interface

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
\class GameStateManager
\brief
	A state machine with an interface specifically for managing game states.
*//******************************************************************/
class GameStateManager : private sm::SimpleStateMachine
{
public:
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
		Shuts down the state machine.
	*//******************************************************************/
	void Exit();
};

#pragma endregion // Interface

#pragma region Definition

template<typename GameStateType>
void GameStateManager::SwitchToState()
{
	SimpleStateMachine::SwitchToState(new GameStateType{});
}

#pragma endregion Definition
