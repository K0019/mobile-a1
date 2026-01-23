/******************************************************************************/
/*!
\file   AnimatorStateMachine.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   11/02/2024

\author Elijah Teo (100%)
\par    email: teo.e\@digipen.edu
\par    DigiPen login: teo.e

\brief
  This is an interface file for a set of classes implementing an animation
  state machine that integrates with the AnimationComponent system.

  This provides the BASE CLASSES for creating animation states, activities,
  and transitions. Specific animation states should be defined elsewhere.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "Engine/StateMachine.h"
#include "Engine/Resources/ResourcesHeader.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

#define ANIMATIONS \
X(IDLE,     "Idle")\
X(WALK,     "Walk")\
X(ATTACK,   "Attack")\
X(HURT,   "Hurt")\
X(DODGE,   "Dodge")\
X(PARRY,   "Parry")\
X(THROW,   "Throw")\

#define X(type, name) type,
enum ANIMATION_TYPES :size_t
{
	ANIMATIONS
	ANIM_TOTAL
};
#undef X

namespace sm {



	//======================================================================
// FORWARD DECLARATIONS - Declare everything first
//======================================================================

// States
	class IdleState;
	class WalkState;
	class AttackState;
	class HurtState;
	class DodgeState;
	class ParryState;
	class ThrowState;

	// Activities
	class IdleActivity;
	class WalkActivity;
	class AttackActivity;
	class HurtActivity;
	class DodgeActivity;
	class ParryActivity;
	class ThrowActivity;

	// Transitions
	class ToIdleTransition;
	class ToWalkTransition;
	class ToAttackTransition;
	class ToHurtTransition;
	class ToDodgeTransition;
	class ToParryTransition;
	class ToThrowTransition;

	

#pragma region Interface

	// Forward declaration
	class AnimStateMachine;

	/*****************************************************************//*!
	\class AnimActivityBase
	\brief
		The base activity class for animation-compatible state machine.
		Inherit from this to create activities that control animations.

	\tparam T
		The final class type of the activity (CRTP pattern).

	\par Example Usage:
		\code
		class PlayIdleActivity : public AnimActivityBase<PlayIdleActivity>
		{
		public:
			void OnEnter(StateMachine* sm) override
			{
				AnimStateMachine* animSM = CastSM(sm);
				// Access entity and set animation here
			}
		};
		\endcode
	*//******************************************************************/
	template <typename T>
	class AnimActivityBase : public ActivityBaseTemplate<T>
	{
	protected:
		/*****************************************************************//*!
		\brief
			Helper function that casts the base state machine pointer
			to an animation-specific state machine pointer.

		\param sm
			A pointer to the base state machine.

		\return
			A pointer to the state machine as an AnimStateMachine.
		*//******************************************************************/
		static AnimStateMachine* CastSM(StateMachine* sm);
	};

	/*****************************************************************//*!
	\class AnimTransitionBase
	\brief
		The base transition class for animation-compatible state machine.
		Inherit from this to create conditions for transitioning between
		animation states.

	\tparam T
		The final class type of the transition (CRTP pattern).

	\par Example Usage:
		\code
		class ToWalkTransition : public AnimTransitionBase<ToWalkTransition>
		{
		public:
			ToWalkTransition()
				: AnimTransitionBase(SET_NEXT_STATE(WalkState)) {}

			bool Decide(StateMachine* sm) override
			{
				AnimStateMachine* animSM = CastSM(sm);
				// Check condition and return true to transition
				return CheckIfShouldWalk(animSM->GetEntity());
			}
		};
		\endcode
	*//******************************************************************/
	template <typename T>
	class AnimTransitionBase : public TransitionBaseTemplate<T>
	{
	public:
		/*****************************************************************//*!
		\brief
			Constructor. Must specify the target state type.

		\tparam StateType
			The state class to transition to when Decide() returns true.

		\param dummy
			The dummy struct containing the state class type.
			Use SET_NEXT_STATE(YourStateType) macro.
		*//******************************************************************/
		template <typename StateType>
		AnimTransitionBase(const TransitionBase::NextStateTypeStruct<StateType>& dummy);

	protected:
		/*****************************************************************//*!
		\brief
			Helper function that casts the base state machine pointer
			to an animation-specific state machine pointer.

		\param sm
			A pointer to the base state machine.

		\return
			A pointer to the state machine as an AnimStateMachine.
		*//******************************************************************/
		static AnimStateMachine* CastSM(StateMachine* sm);
	};

	/*****************************************************************//*!
	\class AnimStateMachine
	\brief
		An animation-compatible state machine manager that provides
		access to the entity's AnimationComponent for controlling animations.

	\par Usage:
		1. Create states inheriting from sm::State
		2. Create activities inheriting from AnimActivityBase<T>
		3. Create transitions inheriting from AnimTransitionBase<T>
		4. Initialize: new AnimStateMachine(new YourStartingState())
		5. Each frame: animStateMachine->Update(entityHandle)

	\par Example:
		\code
		// In your character controller
		AnimStateMachine* animSM = new AnimStateMachine(new IdleState());

		// Each frame
		animSM->Update(characterEntity);
		\endcode
	*//******************************************************************/
	class AnimStateMachine : public StateMachine
	{
	public:
		AnimStateMachine();

		AnimStateMachine(State* startingState);

		AnimStateMachine(UserResourceHandle<ResourceAnimation> inAnim[7], State* startingState);
		/*****************************************************************//*!
		\brief
			Constructor.

		\param startingState
			The initial state of this state machine.
			Ownership is transferred to the state machine.
		*//******************************************************************/
		//AnimStateMachine(State* startingState);

		/*****************************************************************//*!
		\brief
			Updates the state machine, executing activities and checking
			for state transitions. This version provides access to the
			entity for animation control.

		\param thisEntity
			The entity that this state machine is executing on.
			This allows activities and transitions to access the entity's
			AnimationComponent and other components.
		*//******************************************************************/
		void Update(ecs::EntityHandle thisEntity);

		/*****************************************************************//*!
		\brief
			Gets the entity that this state machine is currently executing on.

		\return
			The entity handle. Can be used to access components like
			AnimationComponent, Transform, etc.
		*//******************************************************************/
		ecs::EntityHandle GetEntity() const;
		
		void ResetFlags();

		bool idle = false;
		bool walking = false;
		bool attack = false;
		bool hurt = false;
		bool dodge = false;
		bool parry = false;
		bool throwFlag = false;
		float attackDelay = 0.f;
		UserResourceHandle<ResourceAnimation> animations[ANIMATION_TYPES::ANIM_TOTAL];

		void ChangAnim(uint32_t anim, size_t hash);

	private:
		//! Hide base class update function to enforce entity parameter
		using StateMachine::Update;

		//! The entity that this state machine is executing on
		ecs::EntityHandle entity;

	};

#pragma endregion // Interface

#pragma region Definitions

	template<typename T>
	AnimStateMachine* AnimActivityBase<T>::CastSM(StateMachine* sm)
	{
		return static_cast<AnimStateMachine*>(sm);
	}

	template<typename T>
	template<typename StateType>
	AnimTransitionBase<T>::AnimTransitionBase(const TransitionBase::NextStateTypeStruct<StateType>& dummy)
		: TransitionBaseTemplate<T>{ dummy }
	{
	}

	template<typename T>
	AnimStateMachine* AnimTransitionBase<T>::CastSM(StateMachine* sm)
	{
		return static_cast<AnimStateMachine*>(sm);
	}

#pragma endregion // Definitions

	//======================================================================
	// ACTIVITY DECLARATIONS
	//======================================================================

	class IdleActivity : public sm::AnimActivityBase<IdleActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};

	class WalkActivity : public sm::AnimActivityBase<WalkActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};

	class AttackActivity : public sm::AnimActivityBase<AttackActivity>
	{
		public:
		void OnEnter(sm::StateMachine* sm) override;
		void OnUpdate(sm::StateMachine* sm) override;
	};

	class HurtActivity : public sm::AnimActivityBase<HurtActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};

	class DodgeActivity : public sm::AnimActivityBase<DodgeActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};

	class ParryActivity : public sm::AnimActivityBase<ParryActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};

	class ThrowActivity : public sm::AnimActivityBase<ThrowActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
	};
	

	//======================================================================
	// TRANSITION DECLARATIONS
	//======================================================================

	class ToIdleTransition : public sm::AnimTransitionBase<ToIdleTransition>
	{
	public:
		ToIdleTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToWalkTransition : public sm::AnimTransitionBase<ToWalkTransition>
	{
	public:
		ToWalkTransition();
		bool Decide(sm::StateMachine* sm) override;
	};


	class ToAttackTransition : public sm::AnimTransitionBase<ToAttackTransition>
	{
	public:
		ToAttackTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToHurtTransition : public sm::AnimTransitionBase<ToHurtTransition>
	{
	public:
		ToHurtTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToDodgeTransition : public sm::AnimTransitionBase<ToDodgeTransition>
	{
	public:
		ToDodgeTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToParryTransition : public sm::AnimTransitionBase<ToParryTransition>
	{
	public:
		ToParryTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToThrowTransition : public sm::AnimTransitionBase<ToThrowTransition>
	{
	public:
		ToThrowTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	//======================================================================
	// STATE DECLARATIONS
	//======================================================================

	class IdleState : public sm::State
	{
	public:
		IdleState();
	};

	class WalkState : public sm::State
	{
	public:
		WalkState();
	};


	class AttackState : public sm::State
	{
		public:
		AttackState();
	};

	class HurtState : public sm::State
	{
	public:
		HurtState();
	};

	class DodgeState : public sm::State
	{
	public:
		DodgeState();
	};

	class ParryState : public sm::State
	{
	public:
		ParryState();
	};

	class ThrowState : public sm::State
	{
	public:
		ThrowState();
	};

}