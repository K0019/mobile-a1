/******************************************************************************/
/*!
\file   StateMachine.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is the source file implementing a set of classes implementing a state machine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "StateMachine.h"

namespace sm {

#pragma region Simple

	SimpleStateMachine::SimpleStateMachine()
		: currentState{ nullptr }
	{
	}

	SimpleStateMachine::~SimpleStateMachine()
	{
		if (currentState)
			delete currentState;
	}

	void SimpleStateMachine::SwitchToState(SimpleState* state)
	{
		if (currentState)
		{
			currentState->OnExit();
			delete currentState;
		}

		currentState = state;

		if (currentState)
			currentState->OnEnter();
	}

	void SimpleStateMachine::Update()
	{
		if (currentState)
			currentState->OnUpdate();
	}

#pragma endregion // Simple

#pragma region Full

	State::State(const std::initializer_list<ActivityBase*>& activities, const std::initializer_list<TransitionBase*>& transitions)
		: nextState{ nullptr }
		, activities{ activities }
		, transitions{ transitions }
	{
	}

	State::State(const State& other)
		: nextState{ other.nextState ? new State{ *other.nextState } : nullptr }
	{
		for (ActivityBase* activity : other.activities)
			activities.push_back(activity->Clone());
		for (TransitionBase* transition : other.transitions)
			transitions.push_back(transition->Clone());
	}

	State::State(State&& other) noexcept
		: nextState{ other.nextState }
		, activities{ std::move(other.activities) }
		, transitions{ std::move(other.transitions) }
	{
		other.nextState = nullptr;
	}

	State::~State()
	{
		for (ActivityBase* activity : activities)
			delete activity;
		for (TransitionBase* transition : transitions)
			delete transition;
	}

	void State::OnEnter(StateMachine* sm)
	{
		for (ActivityBase* activity : activities)
			activity->OnEnter(sm);
	}

	void State::OnUpdate(StateMachine* sm)
	{
		// If a transition has already set a next state, don't check for transitions again.
		if (nextState)
			return;

		for (ActivityBase* activity : activities)
			activity->OnUpdate(sm);

		for (TransitionBase* transition : transitions)
			if (transition->Decide(sm))
			{
				transition->SetNextState(&nextState);
				// Prioritize the first transition
				return;
			}
	}

	void State::OnExit(StateMachine* sm)
	{
		for (ActivityBase* activity : activities)
			activity->OnExit(sm);
	}

	bool State::HasNextState() const
	{
		return nextState;
	}

	State* State::ExtractNextState()
	{
		State* returnVal{ nextState };
		nextState = nullptr;
		return returnVal;
	}

	StateMachine::StateMachine(State* startingState)
		: currState{ startingState }
		, isFirstUpdate{ true }
	{
	}

	StateMachine::StateMachine(const StateMachine& other)
		: currState{ new State{ *other.currState } }
		, isFirstUpdate{ true }
	{
	}

	StateMachine::StateMachine(StateMachine&& other) noexcept
		: currState{ other.currState }
		, isFirstUpdate{ other.isFirstUpdate }
	{
		other.currState = nullptr;
	}

	StateMachine::~StateMachine()
	{
		delete currState;
	}

	void StateMachine::Update()
	{
		if (isFirstUpdate)
		{
			currState->OnEnter(this);
			isFirstUpdate = false;
		}

		currState->OnUpdate(this);

		if (currState->HasNextState())
		{
			currState->OnExit(this);
			State* nextState{ currState->ExtractNextState() };
			delete currState;
			currState = nextState;
			currState->OnEnter(this);
			OnStateChanged();
		}
	}

#pragma endregion // Full

}
