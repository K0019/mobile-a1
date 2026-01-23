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
#include "Physics/Physics.h"
#include "Graphics/AnimationComponent.h"
#include "Engine/Input.h"

namespace sm {
	AnimStateMachine::AnimStateMachine() : StateMachine(new IdleState), entity(nullptr) {
	}

	AnimStateMachine::AnimStateMachine(State* startingState) : StateMachine(startingState), entity(nullptr){

	}

	AnimStateMachine::AnimStateMachine(float inMS, float inRS, float inFric, float inDD, float inDS, float inPT, UserResourceHandle<ResourceAnimation> inAnim[7], State* startingState) :moveSpeed(inMS), rotateSpeed(inRS),
										groundFriction(inFric), dodgeDuration(inDD), dodgeSpeed(inDS), parryTime(inPT), animations(*inAnim), StateMachine(startingState), entity(nullptr)
	{

	}

	void AnimStateMachine::ResetFlags() {
		idle = false;
		walking = false;
		attack = false;
		hurt = false;
		dodge = false;
		parry = false;
		throwFlag = false;
	}

	void AnimStateMachine::ChangAnim(uint32_t anim, size_t hash) {
		animations[anim] = hash;
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

	void IdleActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		std::cout << "Entered Idle Activity" << std::endl;
		if (animComp)
		{
			// TODO: Set your actual idle animation hash
			if (animSM->animations[IDLE].GetHash()) {
				animComp->TransitionTo(animSM->animations[IDLE].GetHash(),0.15f);
				//animComp->animHandleA = 1058861583856284945;
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = 1.0f;
				std::cout << "Set Idle Animation" << std::endl;
			}
		}
	}

	//void IdleActivity::OnUpdate(sm::StateMachine* sm)
	//{
	//	// Optionally implement any per-frame logic for the Idle activity here
	//	sm::AnimStateMachine* animSM = CastSM(sm);
	//	if (animSM->attack) {
	//		
	//	}

	//}

	void WalkActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		std::cout << "Entered Walk Activity" << std::endl;
		if (animComp)
		{
			// TODO: Set your actual walk animation hash
			if (animSM->animations[WALK].GetHash()) {
				animComp->TransitionTo(animSM->animations[WALK].GetHash(),0.15f);
				//animComp->animHandleA = 2370177633938117279;
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = 1.0f;
			}
		}
	}

	//void WalkActivity::OnUpdate(sm::StateMachine* sm)
	//{
	//	// Optionally implement any per-frame logic for the Idle activity here
	//	sm::AnimStateMachine* animSM = CastSM(sm);

	//}

	void AttackActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		std::cout << "Entered Attack Activity" << std::endl;
		if (animComp)
		{
			// TODO: Set your actual run animation hash
			if (animSM->animations[ATTACK].GetHash()) {
				animComp->TransitionTo(animSM->animations[ATTACK].GetHash(),0.1f);
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = 1.0f;
				std::cout << "Set Run Animation" << std::endl;
			}
		}
	}

	void HurtActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[HURT].GetHash())
		{
			animComp->TransitionTo(animSM->animations[HURT].GetHash(),0.1f);
			animComp->isPlaying = true;
			animComp->loop = false; // Usually hurt doesn't loop
			animComp->speed = 1.0f;
			std::cout << "Set Hurt Animation" << std::endl;
		}
	}

	void DodgeActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[DODGE].GetHash())
		{
			animComp->TransitionTo(animSM->animations[DODGE].GetHash(),0.05f);
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = 1.0f;
			std::cout << "Set Dodge Animation" << std::endl;
		}
	}

	void ParryActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[PARRY].GetHash())
		{
			animComp->TransitionTo(animSM->animations[PARRY].GetHash(),0.05f);
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = 1.0f;
			std::cout << "Set Parry Animation" << std::endl;
		}
	}

	void ThrowActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[THROW].GetHash())
		{
			animComp->TransitionTo(animSM->animations[THROW].GetHash());
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = 1.0f;
			std::cout << "Set Throw Animation" << std::endl;
		}
	}

	//======================================================================
	// TRANSITION DEFINITIONS
	//======================================================================

	ToIdleTransition::ToIdleTransition()
		: sm::AnimTransitionBase<ToIdleTransition>(SET_NEXT_STATE(IdleState))
	{
	}

	bool ToIdleTransition::Decide(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->idle) {
			return true;
		}
		else {
			animSM->idle = false;
			return false;
		}
		//return animSM->idle;
	}

	ToWalkTransition::ToWalkTransition()
		: sm::AnimTransitionBase<ToWalkTransition>(SET_NEXT_STATE(WalkState))
	{
	}

	bool ToWalkTransition::Decide(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		//float speed = GetEntitySpeed(animSM->GetEntity());
		if (animSM->walking) {
			return true;
		}
		else {
			animSM->walking = false;
			return false;
		}
		//return animSM->walking;//speed > 0.1f && speed < 5.0f;
	}

	ToAttackTransition::ToAttackTransition()
		: sm::AnimTransitionBase<ToAttackTransition>(SET_NEXT_STATE(AttackState))
	{
	}

	bool ToAttackTransition::Decide(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->attack) {
			return true;
		}
		else {
			animSM->attack = false;
			return false;
		}
		//return animSM->attack;
	}

	ToHurtTransition::ToHurtTransition() 
		: sm::AnimTransitionBase<ToHurtTransition>(SET_NEXT_STATE(HurtState)) {}

	bool ToHurtTransition::Decide(sm::StateMachine* sm) {
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->hurt) {
			return true;
		}
		else {
			animSM->hurt = false;
			return false;
		}
	}

	ToDodgeTransition::ToDodgeTransition() 
		: sm::AnimTransitionBase<ToDodgeTransition>(SET_NEXT_STATE(DodgeState)) {}

	bool ToDodgeTransition::Decide(sm::StateMachine* sm) {
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->dodge) {
			return true;
		}
		else {
			animSM->dodge = false;
			return false;
		}
	}

	ToParryTransition::ToParryTransition() 
		: sm::AnimTransitionBase<ToParryTransition>(SET_NEXT_STATE(ParryState)) {}

	bool ToParryTransition::Decide(sm::StateMachine* sm) {
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->parry) {
			return true;
		}
		else {
			animSM->parry = false;
			return false;
		}
	}

	ToThrowTransition::ToThrowTransition() 
		: sm::AnimTransitionBase<ToThrowTransition>(SET_NEXT_STATE(ThrowState)) {}

	bool ToThrowTransition::Decide(sm::StateMachine* sm) {
		sm::AnimStateMachine* animSM = CastSM(sm);
		if (animSM->throwFlag) {
			return true;
		}
		else {
			animSM->throwFlag = false;
			return false;
		}
	}


	//======================================================================
	// STATE DEFINITIONS
	//======================================================================

	IdleState::IdleState() : sm::State(
		{ new IdleActivity() },
		{ new ToWalkTransition(), new ToAttackTransition(), new ToHurtTransition(), new ToDodgeTransition(), new ToParryTransition(), new ToThrowTransition() }
	) {
	}

	WalkState::WalkState() : sm::State(
		{ new WalkActivity() },
		{ new ToIdleTransition(), new ToAttackTransition(), new ToHurtTransition(), new ToDodgeTransition(), new ToParryTransition(), new ToThrowTransition() }
	) {
	}

	AttackState::AttackState() : sm::State(
		{ new AttackActivity() },
		{ new ToIdleTransition(), new ToWalkTransition(), new ToHurtTransition() }
	) {
	}

	HurtState::HurtState() : sm::State(
		{ new HurtActivity() },
		{ new ToIdleTransition() , new ToAttackTransition} // Recover to Idle after being hurt
	) {
	}

	DodgeState::DodgeState() : sm::State(
		{ new DodgeActivity() },
		{ new ToIdleTransition(), new ToWalkTransition() }
	) {
	}

	ParryState::ParryState() : sm::State(
		{ new ParryActivity() },
		{ new ToIdleTransition(), new ToAttackTransition() } // Can counter-attack from parry
	) {
	}

	ThrowState::ThrowState() : sm::State(
		{ new ThrowActivity() },
		{ new ToIdleTransition(), new ToWalkTransition() }
	) {
	}
}