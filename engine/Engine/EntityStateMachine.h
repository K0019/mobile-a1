/******************************************************************************/
/*!
\file   EntityStateMachine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a set of classes specialized for a state
  machine compatible with the ECS architecture.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once

namespace sm {

#pragma region Interface

	// Forward declarations
	class ECSStateMachine;

	/*****************************************************************//*!
	\class ECSActivityBase
	\brief
		The base activity class for ECS compatible state machine.
	\tparam T
		The final class type of the activity.
	*//******************************************************************/
	template <typename T>
	class ECSActivityBase : public ActivityBaseTemplate<T>
	{
	protected:
		/*****************************************************************//*!
		\brief
			Helper function that wraps casting the base state machine pointer
			to an ECS specific state machine pointer into a shorter syntax.
		\param sm
			A pointer to the base state machine.
		\return
			A pointer to the state machine as an ECS compatible state machine.
		*//******************************************************************/
		static ECSStateMachine* CastSM(StateMachine* sm);

	};

	/*****************************************************************//*!
	\class ECSTransitionBase
	\brief
		The base transition class for ECS compatible state machine.
	\tparam T
		The final class type of the transition.
	*//******************************************************************/
	template <typename T>
	class ECSTransitionBase : public TransitionBaseTemplate<T>
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
		ECSTransitionBase(const TransitionBase::NextStateTypeStruct<StateType>& dummy);

	protected:
		/*****************************************************************//*!
		\brief
			Helper function that wraps casting the base state machine pointer
			to an ECS specific state machine pointer into a shorter syntax.
		\param sm
			A pointer to the base state machine.
		\return
			A pointer to the state machine as an ECS compatible state machine.
		*//******************************************************************/
		static ECSStateMachine* CastSM(StateMachine* sm);

	};

	/*****************************************************************//*!
	\class ECSStateMachine
	\brief
		An ECS compatible state machine manager.
	*//******************************************************************/
	class ECSStateMachine : public StateMachine
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor.
		\param startingState
			The initial state of this state machine.
		*//******************************************************************/
		ECSStateMachine(State* startingState);

		/*****************************************************************//*!
		\brief
			Updates the state machine, executing activities and checking
			for state transitions.
		\param thisEntity
			The entity that this state machine is executing on.
		*//******************************************************************/
		void Update(ecs::EntityHandle thisEntity);

		/*****************************************************************//*!
		\brief
			Gets the entity that this state machine is executing on.
		\return
			The entity that this state machine is executing on.
		*//******************************************************************/
		ecs::EntityHandle GetEntity() const;

	private:
		//! Hide previous update function
		using StateMachine::Update;

		//! The entity taht this state machine is executing on.
		ecs::EntityHandle entity;
	};

#pragma endregion // Interface

#pragma region Definitions

	template<typename T>
	ECSStateMachine* ECSActivityBase<T>::CastSM(StateMachine* sm)
	{
		return static_cast<ECSStateMachine*>(sm);
	}

	template<typename T>
	template<typename StateType>
	ECSTransitionBase<T>::ECSTransitionBase(const TransitionBase::NextStateTypeStruct<StateType>& dummy)
		: TransitionBaseTemplate<T>{ dummy }
	{
	}

	template<typename T>
	ECSStateMachine* ECSTransitionBase<T>::CastSM(StateMachine* sm)
	{
		return static_cast<ECSStateMachine*>(sm);
	}

#pragma endregion // Definitions

}

