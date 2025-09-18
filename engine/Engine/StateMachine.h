/******************************************************************************/
/*!
\file   StateMachine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a set of classes implementing a state machine.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Singleton.h"
#include <string>
#include <vector>
#include <unordered_map>

#define SET_NEXT_STATE(nextStateType) \
sm::TransitionBase::NextStateTypeStruct<nextStateType>{}

namespace sm {

#pragma region Simple

	/*****************************************************************//*!
	\class SimpleState
	\brief
		The base of a state class compatible with SimpleStateMachine.
	*//******************************************************************/
	class SimpleState
	{
	public:
		/*****************************************************************//*!
		\brief
			Executed when the state machine switches to this state.
		*//******************************************************************/
		virtual void OnEnter() {};

		/*****************************************************************//*!
		\brief
			Executed when the state machine is updated.
		*//******************************************************************/
		virtual void OnUpdate() {};

		/*****************************************************************//*!
		\brief
			Executed when the state machine switches from this state.
		*//******************************************************************/
		virtual void OnExit() {};
		
	};

	/*****************************************************************//*!
	\class SimpleStateMachine
	\brief
		A simple state machine that requires external modifications for
		state switching.
	*//******************************************************************/
	class SimpleStateMachine
	{
	public:
		/*****************************************************************//*!
		\brief
			Default constructor.
		*//******************************************************************/
		SimpleStateMachine();

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~SimpleStateMachine();

		/*****************************************************************//*!
		\brief
			Switches the current state of the state machine to the specified state.
		\param state
			A pointer to heap allocated state.
		*//******************************************************************/
		void SwitchToState(SimpleState* state);

		/*****************************************************************//*!
		\brief
			Updates the current state.
		*//******************************************************************/
		void Update();

	private:
		//! The current state.
		SimpleState* currentState;

	};

#pragma endregion // Simple

#pragma region Full

#pragma region Interface

	class State;
	class StateMachine;

	/*****************************************************************//*!
	\class ActivityBase
	\brief
		The base class of an activity of a state.
	*//******************************************************************/
	class ActivityBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		virtual ~ActivityBase() = default;

		/*****************************************************************//*!
		\brief
			Executed when the state machine switches to this state.
		\param StateMachine*
			The state machine that this activity is operating under.
		*//******************************************************************/
		virtual void OnEnter(StateMachine*) {};

		/*****************************************************************//*!
		\brief
			Executed when the state machine is updated.
		\param StateMachine*
			The state machine that this activity is operating under.
		*//******************************************************************/
		virtual void OnUpdate(StateMachine*) {};

		/*****************************************************************//*!
		\brief
			Executed when the state machine switches from this state.
		\param StateMachine*
			The state machine that this activity is operating under.
		*//******************************************************************/
		virtual void OnExit(StateMachine*) {};

		/*****************************************************************//*!
		\brief
			Creates a copy of this activity.
		\return
			A copy of this activity.
		*//******************************************************************/
		virtual ActivityBase* Clone() = 0;
	};

	/*****************************************************************//*!
	\class ActivityBaseTemplate
	\brief
		The base class of an activity of a state, implementing the clone
		function automatically for child activity classes.
	\tparam T
		The type of the child activity class.
	*//******************************************************************/
	template <typename T>
	class ActivityBaseTemplate : public ActivityBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Creates a copy of this activity.
		\return
			A copy of this activity.
		*//******************************************************************/
		ActivityBase* Clone() override final;
	};

	/*****************************************************************//*!
	\class TransitionBase
	\brief
		The base class of a transition of a state.
	*//******************************************************************/
	class TransitionBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		virtual ~TransitionBase() = default;

		/*****************************************************************//*!
		\struct NextStateTypeStruct
		\brief
			A dummy struct to facilitate passing a class type to the constructor.
		\tparam StateType
			The state class to transition to when this transition returns true.
		*//******************************************************************/
		template <typename StateType> requires std::derived_from<StateType, State>
		struct NextStateTypeStruct { };

		/*****************************************************************//*!
		\brief
			Constructor.
		\tparam StateType
			The state class to transition to when this transition returns true.
		*//******************************************************************/
		template <typename StateType>
		TransitionBase(const NextStateTypeStruct<StateType>&);

		/*****************************************************************//*!
		\brief
			Tests whether to do this transition when the state updates.
		\param StateMachine*
			The state machine that this transition is operating under.
		*//******************************************************************/
		virtual bool Decide(StateMachine*) = 0;

		/*****************************************************************//*!
		\brief
			Creates a copy of this transition.
		\return
			A copy of this transition.
		*//******************************************************************/
		virtual TransitionBase* Clone() = 0;

		/*****************************************************************//*!
		\brief
			Sets a pointer to point to the next state.
		\param outNextState
			A pointer to the pointer that points to the next state.
		*//******************************************************************/
		void (*const SetNextState)(State** outNextState);
	};

	/*****************************************************************//*!
	\class TransitionBaseTemplate
	\brief
		The base class of an transition of a state, implementing the clone
		function automatically for child transition classes.
	\tparam T
		The type of the child transition class.
	*//******************************************************************/
	template <typename T>
	class TransitionBaseTemplate : public TransitionBase
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\tparam StateType
			The state class to transition to when this transition returns true.
		\param dummy
			The dummy struct that contains the state class type.
		*//******************************************************************/
		template <typename StateType>
		TransitionBaseTemplate(const NextStateTypeStruct<StateType>& dummy);

		/*****************************************************************//*!
		\brief
			Creates a copy of this transition.
		\return
			A copy of this transition.
		*//******************************************************************/
		TransitionBase* Clone() override final;
	};

	/*****************************************************************//*!
	\class State
	\brief
		Contains activities that give this state behaviors, and transitions
		that provide pathways for switching states.
	*//******************************************************************/
	class State
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor
		\param activities
			The activities that this state has.
		\param transitions
			The transitions that this state has.
		*//******************************************************************/
		State(const std::initializer_list<ActivityBase*>& activities, const std::initializer_list<TransitionBase*>& transitions);

		/*****************************************************************//*!
		\brief
			Copy constructor.
		\parma other
			The state to copy.
		*//******************************************************************/
		State(const State& other);

		/*****************************************************************//*!
		\brief
			Move constructor.
		\parma other
			The state to move.
		*//******************************************************************/
		State(State&& other) noexcept;

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~State();

		/*****************************************************************//*!
		\brief
			Executed when the state machine switches to this state.
		\param sm
			The state machine that this state is operating under.
		*//******************************************************************/
		void OnEnter(StateMachine* sm);

		/*****************************************************************//*!
		\brief
			Executed when the state machine is updated.
		\param sm
			The state machine that this state is operating under.
		*//******************************************************************/
		void OnUpdate(StateMachine* sm);

		/*****************************************************************//*!
		\brief
			Executed when the state machine switches from this state.
		\param sm
			The state machine that this state is operating under.
		*//******************************************************************/
		void OnExit(StateMachine* sm);

		/*****************************************************************//*!
		\brief
			Checks if this state has queued a new state for the state machine
			to switch to.
		\return
			True if there is a new state to switch to. False otherwise.
		*//******************************************************************/
		bool HasNextState() const;

		/*****************************************************************//*!
		\brief
			Transfers ownership of the next state from this state to the callee.
		\return
			A pointer to the next state.
		*//******************************************************************/
		State* ExtractNextState();

	protected:
		/*****************************************************************//*!
		\brief
			Sets the next state pointer.
		\tparam T
			The next state's class type.
		*//******************************************************************/
		template <typename T> requires std::derived_from<T, State>
		void SetNextState();

	private:
		//! The next state that the state machine should switch to.
		State* nextState;

		//! The activities that this state has.
		std::vector<ActivityBase*> activities;
		//! The transitions that this state has.
		std::vector<TransitionBase*> transitions;

	};

	/*****************************************************************//*!
	\class StateMachine
	\brief
		Implements a full self-contained stateful state machine.
	*//******************************************************************/
	class StateMachine
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\parma startingState
			A pointer a heap allocated first state.
		*//******************************************************************/
		StateMachine(State* startingState);

		/*****************************************************************//*!
		\brief
			Copy constructor.
		\parma other
			The state machine to copy.
		*//******************************************************************/
		StateMachine(const StateMachine& other);

		/*****************************************************************//*!
		\brief
			Move constructor.
		\parma other
			The state machine to move.
		*//******************************************************************/
		StateMachine(StateMachine&& other) noexcept;

		/*****************************************************************//*!
		\brief
			Destructor.
		*//******************************************************************/
		~StateMachine();

		/*****************************************************************//*!
		\brief
			Updates the current state, and transitions to a new state if necessary.
		*//******************************************************************/
		void Update();

	protected:
		/*****************************************************************//*!
		\brief
			Called when the state is changed. Can be overriden by child specializations
			of this class.
		*//******************************************************************/
		virtual void OnStateChanged() {};

	private:
		//! The current state
		State* currState;
		//! Whether it is the first time updating.
		bool isFirstUpdate;

	};

#pragma endregion // Interface

#pragma region Definition

	template<typename T>
	ActivityBase* ActivityBaseTemplate<T>::Clone()
	{
		return new T{ static_cast<const T&>(*this) };
	}

	template<typename StateType>
	TransitionBase::TransitionBase(const NextStateTypeStruct<StateType>&)
		: SetNextState{ [](State** outNextState) -> void {
			*outNextState = new StateType{};
		} }
	{
	}

	template<typename T>
	template<typename StateType>
	TransitionBaseTemplate<T>::TransitionBaseTemplate(const NextStateTypeStruct<StateType>& dummy)
		: TransitionBase{ dummy }
	{
	}

	template<typename T>
	TransitionBase* TransitionBaseTemplate<T>::Clone()
	{
		return new T{ static_cast<const T&>(*this) };
	}

	template<typename T> requires std::derived_from<T, State>
	void State::SetNextState()
	{
		if (nextState)
			delete nextState;
		nextState = new T{};
	}

#pragma endregion // Definition

#pragma endregion // Full

}
