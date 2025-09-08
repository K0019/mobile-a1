/******************************************************************************/
/*!
\file   GameStateMachine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a set of classes implementing state machine behaviors
  for use in game.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "EntityStateMachine.h"

namespace sm {

#pragma region Common Transitions

	/*****************************************************************//*!
	\class TransitionAlways
	\brief
		Always signals for a state change.
	*//******************************************************************/
	class TransitionAlways : public TransitionBaseTemplate<TransitionAlways>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param dummy
			Dummy structure to pass the type of the next state to transition to.
		\param delay
			The amount of time until signaling for a state change.
		*//******************************************************************/
		template <typename NextStateType>
		TransitionAlways(const NextStateTypeStruct<NextStateType>& dummy);

		/*****************************************************************//*!
		\brief
			Tests whether to change state.
		\param sm
			The state machine.
		\return
			True when it's time to change state. False otherwise.
		*//******************************************************************/
		virtual bool Decide(StateMachine* sm) override;
	};

	template<typename NextStateType>
	TransitionAlways::TransitionAlways(const NextStateTypeStruct<NextStateType>& dummy)
		: TransitionBaseTemplate{ dummy }
	{
	}

	/*****************************************************************//*!
	\class TransitionTimed
	\brief
		Signals for a state change after a certain amount of time.
	*//******************************************************************/
	class TransitionTimed : public TransitionBaseTemplate<TransitionTimed>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param dummy
			Dummy structure to pass the type of the next state to transition to.
		\param delay
			The amount of time until signaling for a state change.
		*//******************************************************************/
		template <typename NextStateType>
		TransitionTimed(const NextStateTypeStruct<NextStateType>& dummy, float delay);

		/*****************************************************************//*!
		\brief
			Tests whether to change state.
		\param sm
			The state machine.
		\return
			True when it's time to change state. False otherwise.
		*//******************************************************************/
		virtual bool Decide(StateMachine* sm) override;

	private:
		//! Time left until state change.
		float timer;
	};

	template<typename NextStateType>
	TransitionTimed::TransitionTimed(const NextStateTypeStruct<NextStateType>& dummy, float delay)
		: TransitionBaseTemplate{ dummy }
		, timer{ delay }
	{
	}

#pragma endregion // Common Transitions

#pragma region Activities



#pragma endregion // Activities

#pragma region States



#pragma endregion // States

}
