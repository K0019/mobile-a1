/******************************************************************************/
/*!
\file   CharacterAnimStates.h
\brief  Example implementation of an animation state machine for a character

This example shows how to create states and transitions for character animations
like Idle, Walk, Run, and Jump.
*/
/******************************************************************************/

#pragma once
#include "Engine/AnimatorStateMachine.h"
#include "Utilities/GameTime.h"
#include "Physics/Physics.h"
#include "Graphics/AnimationComponent.h"
#include "Engine/Input.h"
#include "Game/Character.h"

static constexpr float ANIM_TRANSITION_DURATION_IDLE = 0.15f;
static constexpr float ANIM_TRANSITION_DURATION_ATTACK = 0.1f;
static constexpr float ANIM_TRANSITION_DURATION_WALK = 0.15f;
static constexpr float ANIM_TRANSITION_DURATION_PARRY = 0.1f;
static constexpr float ANIM_TRANSITION_DURATION_HURT = 0.05f;
static constexpr float ANIM_TRANSITION_DURATION_DODGE = 0.05f;
static constexpr float ANIM_TRANSITION_DURATION_THROW = 0.1f;

#define X(type, name) name,
static const char* const ANIM_INPUT_KEYS[]{
	ANIM_INPUT_TYPE_ENUM
};

namespace sm {

	constexpr const char* AnimInputTypeToKey(ANIM_INPUT_TYPE type)
	{
		return ANIM_INPUT_KEYS[+type];
	}

	AnimStateMachine::AnimStateMachine() : StateMachine(new IdleState), entity(nullptr) {
	}

	AnimStateMachine::AnimStateMachine(State* startingState) : StateMachine(startingState), entity(nullptr){

	}

	void AnimStateMachine::Update(ecs::EntityHandle thisEntity)
	{
		entity = thisEntity;

		// Call the base class Update() which handles all the state logic
		StateMachine::Update();

		// Clear entity reference after update (optional, for safety)
		// entity = nullptr;  // Uncomment if you want to be extra safe
	}

	ecs::EntityHandle AnimStateMachine::GetEntity() const {
		return entity;
	}
	//======================================================================
	// HELPER FUNCTIONS (can be inline)
	//======================================================================

	inline Vec3 GetEntityVelo(ecs::EntityHandle entity)
	{
		
		Vec3 entityVelo = entity->GetComp<physics::JoltBodyComp>()->GetLinearVelocity();
		// TODO: Implement your speed calculation here
		// Example: 
		// if (auto* transform = entity->GetComp<Transform>())
		//     return glm::length(transform->velocity);
		return entityVelo;
	}

	//======================================================================
	// ACTIVITY DEFINITIONS
	//======================================================================

	// Helper function to obtain the WeaponInfo of the character
	const WeaponInfo* GetWeaponInfo(sm::AnimStateMachine* sm)
	{
		ecs::EntityHandle charEntity{ sm->GetEntity() };
		if (auto charComp{ charEntity->GetComp<CharacterMovementComponent>() })
			if (auto itemComp{ charComp->GetHeldItem() })
				if (auto weaponInfo{ itemComp->weaponInfo.GetResource() })
					return weaponInfo;
		return nullptr;
	}

	// Helper function for activities to transition into a new animation as described inside WeaponInfo
	bool TransitionChracterIntoAnimation(sm::AnimStateMachine* sm, size_t animIndex, float animTransitionDuration, bool loop = true)
	{
		const WeaponInfo* weaponInfo{ GetWeaponInfo(sm) };
		if (!weaponInfo)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "AnimatorStateMachine could not find WeaponInfo, or it is invalid";
			return false;
		}
		if (animIndex >= weaponInfo->moves.size())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "WeaponInfo doesn't have enough moves, unable to determine animation to transition to";
			return false;
		}

		ecs::EntityHandle charEntity{ sm->GetEntity() };
		auto animComp{ charEntity->GetComp<AnimationComponent>() };
		if (!animComp)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "AnimatorStateMachine cannot find AnimationComponent";
			return false;
		}

		animComp->TransitionTo(weaponInfo->moves[animIndex].anim.GetHash(), animTransitionDuration);
		animComp->isPlaying = true;
		animComp->loop = loop;
		return true;
	}

	void IdleActivity::OnEnter(sm::StateMachine* sm)
	{
		TransitionChracterIntoAnimation(CastSM(sm), 0, ANIM_TRANSITION_DURATION_IDLE);
	}

	void WalkActivity::OnEnter(sm::StateMachine* sm)
	{
		TransitionChracterIntoAnimation(CastSM(sm), 1, ANIM_TRANSITION_DURATION_WALK);
	}

	AttackActivity::AttackActivity(size_t moveIndex, ANIM_INPUT_TYPE attackType)
		: moveIndex{ moveIndex }
		, attackType{ attackType }
	{
	}

	void AttackActivity::OnEnter(sm::StateMachine* sm)
	{
		auto animSM{ CastSM(sm) };
		TransitionChracterIntoAnimation(animSM, moveIndex, ANIM_TRANSITION_DURATION_ATTACK, false);
		animSM->blackboard[AnimInputTypeToKey(attackType)] = false; // Consume input
		animSM->blackboard["attacking"] = true; // Mark that we are currently attacking

		// Use this to track whether we've already attacked while we're in the AttackActivity
		animSM->blackboard["attacked"] = false;

		// TODO: Play sound
		//std::string tmpName;
		//auto grabbableComp = attackItem->GetComp<GrabbableItemComponent>();
		//if (grabbableComp->audioStartIndex > grabbableComp->audioEndIndex + 1)
		//	tmpName = grabbableComp->audioName + std::to_string(randomRange(grabbableComp->audioEndIndex + 1, grabbableComp->audioStartIndex));
		//else
		//	tmpName = grabbableComp->audioName + std::to_string(randomRange(grabbableComp->audioStartIndex, grabbableComp->audioEndIndex + 1));

		////if (randomRange(0, 2) == 0)
		//ST<AudioManager>::Get()->PlaySound3D(tmpName, false, ecs::GetEntity(this)->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);
	}
	void AttackActivity::OnUpdate(sm::StateMachine* sm)
	{
		// Don't need to do anything if already applied hitbox attack
		auto animSM{ CastSM(sm) };
		if (animSM->GetBlackboardVal<bool>("attacked"))
			return;

		// We need the WeaponInfo to get the hit parameters
		auto weaponInfo{ GetWeaponInfo(animSM) };
		if (!weaponInfo || weaponInfo->moves.size() <= moveIndex)
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Can't find WeaponInfo or WeaponInfo doesn't have a move at index " << moveIndex << ", unable to apply attack hit logic";
			return;
		}
		const auto& weaponMove{ weaponInfo->moves[moveIndex] };

		// Check if it's time to do the hit
		ecs::EntityHandle charEntity{ animSM->GetEntity() };
		auto animComp{ charEntity->GetComp<AnimationComponent>() };
		if (!animComp)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "AnimatorStateMachine cannot find AnimationComponent";
			return;
		}
		// timeA is the elapsed duration of the attack animation
		if (animComp->timeA < weaponMove.hitDelay)
			return;

		animSM->blackboard["outputApplyHitMove"] = static_cast<int>(moveIndex); // Indicate to the code processing the hit which move index to access
		animSM->blackboard["attacked"] = true;
	}

	void AttackActivity::OnExit(sm::StateMachine* sm)
	{
		CastSM(sm)->blackboard["attacking"] = false;
	}

	void HurtActivity::OnEnter(sm::StateMachine* sm)
	{
		TransitionChracterIntoAnimation(CastSM(sm), 3, ANIM_TRANSITION_DURATION_HURT, false);
	}

	void HurtActivity::OnExit(sm::StateMachine* sm)
	{
		// Only disable hurt input after finishing animation to avoid stun lock via queuing up another hurt animation during a hurt animation
		CastSM(sm)->blackboard["inputHurt"] = false;
	}

	void DodgeActivity::OnEnter(sm::StateMachine* sm)
	{
		auto animSM{ CastSM(sm) };
		TransitionChracterIntoAnimation(animSM, 4, ANIM_TRANSITION_DURATION_DODGE, false);
		animSM->blackboard["inputDodge"] = false;
	}

	void ParryActivity::OnEnter(sm::StateMachine* sm)
	{
		auto animSM{ CastSM(sm) };
		TransitionChracterIntoAnimation(animSM, 5, ANIM_TRANSITION_DURATION_PARRY, false);
		animSM->blackboard["inputParry"] = false;
	}

	void ThrowActivity::OnEnter(sm::StateMachine* sm)
	{
		auto animSM{ CastSM(sm) };
		TransitionChracterIntoAnimation(animSM, 6, ANIM_TRANSITION_DURATION_THROW, false);
		animSM->blackboard["inputThrow"] = false;
	}

	//======================================================================
	// TRANSITION DEFINITIONS
	//======================================================================

	NoOpWhileAnimatingTransition::NoOpWhileAnimatingTransition()
		: sm::AnimTransitionBase<NoOpWhileAnimatingTransition>{ SET_NEXT_STATE(std::nullptr_t) } {}

	bool NoOpWhileAnimatingTransition::Decide(sm::StateMachine* sm)
	{
		// Stop locking transition only once animation is completed
		if (auto animComp{ CastSM(sm)->GetEntity()->GetComp<AnimationComponent>() })
			return animComp->isPlaying;
		else
			// No animation comp, assume no animation playing
			return false;
	}

	NoOpBeforeAttackDamageTransition::NoOpBeforeAttackDamageTransition()
		: sm::AnimTransitionBase<NoOpBeforeAttackDamageTransition>{ SET_NEXT_STATE(std::nullptr_t) } {}

	bool NoOpBeforeAttackDamageTransition::Decide(sm::StateMachine* sm)
	{
		return !CastSM(sm)->GetBlackboardVal<bool>("attacked");
	}


	ToIdleTransition::ToIdleTransition()
		: sm::AnimTransitionBase<ToIdleTransition>(SET_NEXT_STATE(IdleState)) {}

	bool ToIdleTransition::Decide(sm::StateMachine* sm)
	{
		return CastSM(sm)->GetBlackboardVal<Vec2>("inputMovement").LengthSqr() < 0.01f;
	}

	ToWalkTransition::ToWalkTransition()
		: sm::AnimTransitionBase<ToWalkTransition>(SET_NEXT_STATE(WalkState)) {}

	bool ToWalkTransition::Decide(sm::StateMachine* sm)
	{
		return CastSM(sm)->GetBlackboardVal<Vec2>("inputMovement").LengthSqr() >= 0.01f;
	}

	ToHurtTransition::ToHurtTransition() 
		: sm::AnimTransitionBase<ToHurtTransition>(SET_NEXT_STATE(HurtState)) {}

	bool ToHurtTransition::Decide(sm::StateMachine* sm) {
		return CastSM(sm)->GetBlackboardVal<bool>("inputHurt");
	}

	ToDodgeTransition::ToDodgeTransition() 
		: sm::AnimTransitionBase<ToDodgeTransition>(SET_NEXT_STATE(DodgeState)) {}

	bool ToDodgeTransition::Decide(sm::StateMachine* sm) {
		return CastSM(sm)->GetBlackboardVal<bool>("inputDodge");
	}

	ToParryTransition::ToParryTransition() 
		: sm::AnimTransitionBase<ToParryTransition>(SET_NEXT_STATE(ParryState)) {}

	bool ToParryTransition::Decide(sm::StateMachine* sm) {
		return CastSM(sm)->GetBlackboardVal<bool>("inputParry");
	}

	ToThrowTransition::ToThrowTransition() 
		: sm::AnimTransitionBase<ToThrowTransition>(SET_NEXT_STATE(ThrowState)) {}

	bool ToThrowTransition::Decide(sm::StateMachine* sm) {
		return CastSM(sm)->GetBlackboardVal<bool>("inputThrow");
	}


	//======================================================================
	// STATE DEFINITIONS
	//======================================================================

	IdleState::IdleState() : sm::State(
		{ new IdleActivity() },
		{ new ToWalkTransition(), new ToAttackTransition<AttackState>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToHurtTransition(), new ToDodgeTransition(), new ToParryTransition(), new ToThrowTransition() }
	) {
	}

	WalkState::WalkState() : sm::State(
		{ new WalkActivity() },
		{ new ToIdleTransition(), new ToAttackTransition<AttackState>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToHurtTransition(), new ToDodgeTransition(), new ToParryTransition(), new ToThrowTransition() }
	) {
	}

	AttackState::AttackState() : sm::State(
		{ new AttackActivity{ 2, ANIM_INPUT_TYPE::LIGHT_ATTACK } },
		{ new ToHurtTransition{}, new NoOpBeforeAttackDamageTransition{}, new ToAttackTransition<AttackState>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new NoOpWhileAnimatingTransition{}, new ToIdleTransition(), new ToWalkTransition() }
	) {
	}

	HurtState::HurtState() : sm::State(
		{ new HurtActivity() },
		{ new NoOpWhileAnimatingTransition{}, new ToAttackTransition<AttackState>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToIdleTransition() } // Recover to Idle after being hurt
	) {
	}

	DodgeState::DodgeState() : sm::State(
		{ new DodgeActivity() },
		{ new NoOpWhileAnimatingTransition{}, new ToIdleTransition(), new ToWalkTransition() }
	) {
	}

	ParryState::ParryState() : sm::State(
		{ new ParryActivity() },
		{ new NoOpWhileAnimatingTransition{}, new ToIdleTransition(), new ToAttackTransition<AttackState>{ ANIM_INPUT_TYPE::LIGHT_ATTACK } } // Can counter-attack from parry
	) {
	}

	ThrowState::ThrowState() : sm::State(
		{ new ThrowActivity() },
		{ new NoOpWhileAnimatingTransition{}, new ToIdleTransition(), new ToWalkTransition() }
	) {
	}

	LightAttackPlayer1::LightAttackPlayer1() : sm::State{
		{ new AttackActivity{ 2, ANIM_INPUT_TYPE::LIGHT_ATTACK } },
		{ new ToHurtTransition{}, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpBeforeAttackDamageTransition{}, // Allow getting hit or dodging out of the attack animation at any point in time
		  new ToAttackTransition<LightAttackPlayer2>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer1>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new NoOpWhileAnimatingTransition{}, // Allow transitioning to the next attack combo at any point until the end of the current attack animation
		  new ToIdleTransition{}, new ToWalkTransition{} } // No more attack inputs, return to idle/walk
	} {
	}
	LightAttackPlayer2::LightAttackPlayer2() : sm::State{
		{ new AttackActivity{ 3, ANIM_INPUT_TYPE::LIGHT_ATTACK } },
		{ new ToHurtTransition{}, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpBeforeAttackDamageTransition{},
		  new ToAttackTransition<LightAttackPlayer3>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer3>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new NoOpWhileAnimatingTransition{},
		  new ToIdleTransition{}, new ToWalkTransition{} }
	} {
	}
	LightAttackPlayer3::LightAttackPlayer3() : sm::State{
		{ new AttackActivity{ 4, ANIM_INPUT_TYPE::LIGHT_ATTACK } },
		{ new ToHurtTransition{}, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpBeforeAttackDamageTransition{},
		  new ToAttackTransition<LightAttackPlayer4>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer1>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new NoOpWhileAnimatingTransition{},
		  new ToIdleTransition{}, new ToWalkTransition{} }
	} {
	}
	LightAttackPlayer4::LightAttackPlayer4() : sm::State{
		{ new AttackActivity{ 5, ANIM_INPUT_TYPE::LIGHT_ATTACK } },
		{ new ToHurtTransition{}, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpBeforeAttackDamageTransition{},
		  new ToAttackTransition<LightAttackPlayer1>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer1>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new NoOpWhileAnimatingTransition{},
		  new ToIdleTransition{}, new ToWalkTransition{} }
	} {
	}

	HeavyAttackPlayer1::HeavyAttackPlayer1() : sm::State{
		{ new AttackActivity{ 6, ANIM_INPUT_TYPE::HEAVY_ATTACK } },
		{ new ToHurtTransition{}, new NoOpBeforeAttackDamageTransition{}, // No dodging when doing heavy attacks until the attack hits
		  new ToAttackTransition<LightAttackPlayer1>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer2>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpWhileAnimatingTransition{}, // Only allow dodging after hitting the attack
		  new ToIdleTransition{}, new ToWalkTransition{} } // No more attack inputs, return to idle/walk
	} {
	}
	HeavyAttackPlayer2::HeavyAttackPlayer2() : sm::State{
		{ new AttackActivity{ 7, ANIM_INPUT_TYPE::HEAVY_ATTACK } },
		{ new ToHurtTransition{}, new NoOpBeforeAttackDamageTransition{},
		  new ToAttackTransition<LightAttackPlayer1>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer3>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpWhileAnimatingTransition{}, // Only allow dodging after hitting the attack
		  new ToIdleTransition{}, new ToWalkTransition{} }
	} {
	}
	HeavyAttackPlayer3::HeavyAttackPlayer3() : sm::State{
		{ new AttackActivity{ 8, ANIM_INPUT_TYPE::HEAVY_ATTACK } },
		{ new ToHurtTransition{}, new NoOpBeforeAttackDamageTransition{},
		  new ToAttackTransition<LightAttackPlayer1>{ ANIM_INPUT_TYPE::LIGHT_ATTACK }, new ToAttackTransition<HeavyAttackPlayer1>{ ANIM_INPUT_TYPE::HEAVY_ATTACK }, new ToParryTransition{}, new ToDodgeTransition{}, new NoOpWhileAnimatingTransition{}, // Only allow dodging after hitting the attack
		  new ToIdleTransition{}, new ToWalkTransition{} }
	} {
	}
}