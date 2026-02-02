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

#define ANIM_INPUT_TYPE_ENUM \
X(LIGHT_ATTACK, "inputLightAttack")\
X(HEAVY_ATTACK, "inputHeavyAttack")\
X(SKILL_ATTACK, "inputSkillAttack")

enum class ANIM_INPUT_TYPE
{
	ANIM_INPUT_TYPE_ENUM
};
#undef X

namespace sm {

	// Helper function for button input types
	constexpr const char* AnimInputTypeToKey(ANIM_INPUT_TYPE type);

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

		std::unordered_map<std::string, std::any> blackboard;
		template <typename T>
		T GetBlackboardVal(const std::string& key) const;

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
		AttackActivity(size_t moveIndex, ANIM_INPUT_TYPE attackType);

		void OnEnter(sm::StateMachine* sm) override;
		void OnUpdate(sm::StateMachine* sm) override;
		void OnExit(sm::StateMachine* sm) override;

	private:
		size_t moveIndex;
		ANIM_INPUT_TYPE attackType;
	};

	class HurtActivity : public sm::AnimActivityBase<HurtActivity>
	{
	public:
		void OnEnter(sm::StateMachine* sm) override;
		void OnExit(sm::StateMachine* sm) override;
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

	class DecayDelusionIfEnhancedActivity : public sm::AnimActivityBase<DecayDelusionIfEnhancedActivity>
	{
	public:
		void OnUpdate(sm::StateMachine* sm) override;
	};

	

	//======================================================================
	// TRANSITION DECLARATIONS
	//======================================================================

	// Transition overrides all later transitions by doing nothing until the currently playing animation is finished
	class NoOpWhileAnimatingTransition : public sm::AnimTransitionBase<NoOpWhileAnimatingTransition>
	{
	public:
		NoOpWhileAnimatingTransition();
		bool Decide(sm::StateMachine* sm) override;
	};
	// Same thing but only until an attack is dealt (doesn't care about animation)
	class NoOpBeforeAttackDamageTransition : public sm::AnimTransitionBase<NoOpBeforeAttackDamageTransition>
	{
	public:
		NoOpBeforeAttackDamageTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

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
	
	template <typename ToState>
	class ToAttackTransition : public sm::AnimTransitionBase<ToAttackTransition<ToState>>
	{
	private:
		ANIM_INPUT_TYPE attackType;

	public:
		ToAttackTransition(ANIM_INPUT_TYPE attackType)
			: sm::AnimTransitionBase<ToAttackTransition>(SET_NEXT_STATE(ToState))
			, attackType{ attackType }
		{
		}

		bool Decide(sm::StateMachine* sm) override
		{
			return ToAttackTransition<ToState>::CastSM(sm)->template GetBlackboardVal<bool>(AnimInputTypeToKey(attackType));
		}
	};

	// ToSkillAttackTransition defined below, because it needs the states to be defined first
	// Side effect: Sets "enhanced" to true if transition is taken
	class ToSkillAttackTransition;

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

	class ToHitstopTransition : public sm::TransitionBaseTemplate<ToHitstopTransition>
	{
	public:
		ToHitstopTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToPrevStateTransition : public sm::TransitionBase
	{
	public:
		ToPrevStateTransition(State* inPrevState);
		~ToPrevStateTransition();
		bool Decide(sm::StateMachine* sm) override;
		TransitionBase* Clone() override;
	private:
		State* prevState;
	};

	// Side effect: Sets "enhanced" to false if transition is taken
	class OutOfDelusionTransition : public sm::AnimTransitionBase<OutOfDelusionTransition>
	{
	public:
		OutOfDelusionTransition();
		bool Decide(sm::StateMachine* sm) override;
	};



	// Delusion versions of transitions
	class ToDelusionIdleTransition : public sm::AnimTransitionBase<ToDelusionIdleTransition>
	{
	public:
		ToDelusionIdleTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToDelusionWalkTransition : public sm::AnimTransitionBase<ToDelusionWalkTransition>
	{
	public:
		ToDelusionWalkTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	template <typename ToState>
	class ToDelusionAttackTransition : public sm::AnimTransitionBase<ToDelusionAttackTransition<ToState>>
	{
	private:
		ANIM_INPUT_TYPE attackType;

	public:
		ToDelusionAttackTransition(ANIM_INPUT_TYPE attackType)
			: sm::AnimTransitionBase<ToDelusionAttackTransition>(SET_NEXT_STATE(ToState))
			, attackType{ attackType }
		{
		}

		bool Decide(sm::StateMachine* sm) override
		{
			return ToDelusionAttackTransition<ToState>::CastSM(sm)->template GetBlackboardVal<bool>(AnimInputTypeToKey(attackType));
		}
	};

	class ToDelusionHurtTransition : public sm::AnimTransitionBase<ToDelusionHurtTransition>
	{
	public:
		ToDelusionHurtTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToDelusionDodgeTransition : public sm::AnimTransitionBase<ToDelusionDodgeTransition>
	{
	public:
		ToDelusionDodgeTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToDelusionParryTransition : public sm::AnimTransitionBase<ToDelusionParryTransition>
	{
	public:
		ToDelusionParryTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	class ToDelusionThrowTransition : public sm::AnimTransitionBase<ToDelusionThrowTransition>
	{
	public:
		ToDelusionThrowTransition();
		bool Decide(sm::StateMachine* sm) override;
	};

	//======================================================================
	// STATE DECLARATIONS
	//======================================================================

	class IdleState : public sm::State { public: IdleState(); };

	class WalkState : public sm::State { public: WalkState(); };

	class AttackState : public sm::State { public: AttackState(); };

	class HurtState : public sm::State { public: HurtState(); };

	class DodgeState : public sm::State { public: DodgeState(); };

	class ParryState : public sm::State { public: ParryState(); };

	class ThrowState : public sm::State { public: ThrowState(); };

	class HitstopState : public sm::State
	{
	public:
		HitstopState(State* prevState);
	};



	// Delusion versions of states
	class DelusionIdleState : public sm::State { public: DelusionIdleState(); };

	class DelusionWalkState : public sm::State { public: DelusionWalkState(); };

	class DelusionAttackState : public sm::State { public: DelusionAttackState(); };

	class DelusionHurtState : public sm::State { public: DelusionHurtState(); };

	class DelusionDodgeState : public sm::State { public: DelusionDodgeState(); };

	class DelusionParryState : public sm::State{public: DelusionParryState();};

	class DelusionThrowState : public sm::State{public: DelusionThrowState();};

	// Attacks
	class LightAttackPlayer1 : public sm::State { public: LightAttackPlayer1(); };
	class LightAttackPlayer2 : public sm::State { public: LightAttackPlayer2(); };
	class LightAttackPlayer3 : public sm::State { public: LightAttackPlayer3(); };
	class LightAttackPlayer4 : public sm::State { public: LightAttackPlayer4(); };

	class HeavyAttackPlayer1 : public sm::State { public: HeavyAttackPlayer1(); };
	class HeavyAttackPlayer2 : public sm::State { public: HeavyAttackPlayer2(); };
	class HeavyAttackPlayer3 : public sm::State { public: HeavyAttackPlayer3(); };

	class SkillAttackPlayer1 : public sm::State { public: SkillAttackPlayer1(); };

	//delusion attacks
	class DelusionLightAttackPlayer1 : public sm::State { public: DelusionLightAttackPlayer1(); };
	class DelusionLightAttackPlayer2 : public sm::State { public: DelusionLightAttackPlayer2(); };
	class DelusionLightAttackPlayer3 : public sm::State { public: DelusionLightAttackPlayer3(); };
	class DelusionLightAttackPlayer4 : public sm::State { public: DelusionLightAttackPlayer4(); };

	class DelusionHeavyAttackPlayer1 : public sm::State { public: DelusionHeavyAttackPlayer1(); };
	class DelusionHeavyAttackPlayer2 : public sm::State { public: DelusionHeavyAttackPlayer2(); };
	class DelusionHeavyAttackPlayer3 : public sm::State { public: DelusionHeavyAttackPlayer3(); };
}

namespace sm {
	
	// Side effect: Sets "enhanced" to true if transition is taken
	class ToSkillAttackTransition : ToAttackTransition<SkillAttackPlayer1>
	{
	public:
		bool Decide(sm::StateMachine* sm) override;
	};

}

namespace sm {

	template<typename T>
	T AnimStateMachine::GetBlackboardVal(const std::string& key) const
	{
		auto iter{ blackboard.find(key) };
		if (iter == blackboard.end())
			return T{};
		try {
			return std::any_cast<T>(iter->second);
		}
		catch (const std::bad_any_cast&) {
			return T{};
		}
	}

}