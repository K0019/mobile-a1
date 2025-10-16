/******************************************************************************/
/*!
\file   IGameComponentCallbacks.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/25/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the interface file for a component callbacks interface that also provides a
  OnStart() interface that is only called when the simulation is running.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "game.h"

/*****************************************************************//*!
\class IGameComponentCallbacks
\brief
	In addition to the callbacks provided by ecs::IComponentCallbacks, also
	provides an OnStart() callback to components.

	NOTE THAT IF OVERRIDING OnAttached(), ALWAYS CALL BASE OnAttached()!
\tparam CompType
	The type of the component.
*//******************************************************************/
template <typename CompType>
class IGameComponentCallbacks : public virtual ecs::IComponentCallbacks
{
public:
	/*****************************************************************//*!
	\brief
		Called when the component is attached to the entity. Calls OnStart() immediately
		if the simulation has already started.

		IF OVERRIDING THIS, ALWAYS CALL THE BASE!
	*//******************************************************************/
	virtual void OnAttached() override;

	/*****************************************************************//*!
	\brief
		Called when the simulation starts or when the component is attached to an
		entity during the simulation.
	*//******************************************************************/
	virtual void OnStart() {}

private:
	/*****************************************************************//*!
	\brief
		Registers the component type into GameComponentCallbacksHandler.
	\return
		Dummy bool.
	*//******************************************************************/
	static bool RegisterType();

	//! Calls RegisterType() once when this static is initialized at startup
	inline static bool isRegistered{ RegisterType() };

};

/*****************************************************************//*!
\class GameComponentCallbacksHandler
\brief
	Handles calling OnStart() on existing components when the simulation starts.
*//******************************************************************/
class GameComponentCallbacksHandler
{
public:
	/*****************************************************************//*!
	\brief
		Destructor.
	*//******************************************************************/
	~GameComponentCallbacksHandler();

	/*****************************************************************//*!
	\brief
		Registers a component type that is compatible with the callback.
	*//******************************************************************/
	template <typename T>
	void RegisterType();

private:
	friend ST<GameComponentCallbacksHandler>;

	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	GameComponentCallbacksHandler();

	/*****************************************************************//*!
	\brief
		Called by messaging system when the simulation starts. Calls OnStart()
		on all existing components that support the callback.
	*//******************************************************************/
	static void OnEngineSimulationStart();

private:
	//! Vector of functions that call callbacks that should be called when the simulation starts on all existing components
	std::vector<void(*)()> retroactiveCallbacks;
};

template <typename CompType>
void IGameComponentCallbacks<CompType>::OnAttached()
{
	// If the game has already started, call OnStart() immediately
	// But don't call on start if the ecs dimension isn't the default pool
	if (ecs::GetCurrentPoolId() == ecs::POOL::DEFAULT &&
		ST<Game>::Get()->GetState() != GAMESTATE::EDITOR)
	{
		OnStart();
	}
}

template<typename CompType>
bool IGameComponentCallbacks<CompType>::RegisterType()
{
	ST<GameComponentCallbacksHandler>::Get()->RegisterType<CompType>();
	return true;
}

template<typename CompType>
void GameComponentCallbacksHandler::RegisterType()
{
	retroactiveCallbacks.push_back([]() -> void {
		// Calls required callbacks for each existing component of the specified type.
		for (auto compIter{ ecs::GetCompsBegin<CompType>() }, endIter{ ecs::GetCompsEnd<CompType>() }; compIter != endIter; ++compIter)
			compIter->OnStart();
	});
}
