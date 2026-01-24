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

namespace sm {
	AnimStateMachine::AnimStateMachine() : StateMachine(new IdleState), entity(nullptr) {
	}

	AnimStateMachine::AnimStateMachine(State* startingState) : StateMachine(startingState), entity(nullptr){

	}

	AnimStateMachine::AnimStateMachine(UserResourceHandle<ResourceAnimation> inAnim[7], State* startingState) : animations(*inAnim), StateMachine(startingState), entity(nullptr)
	{
		animSwitch["IDLE"] = false;
		animSwitch["ATTACK"] = false;
		animSwitch["WALK"] = false;
		animSwitch["PARRY"] = false;
		animSwitch["HURT"] = false;
		animSwitch["DODGE"] = false;
		animSwitch["THROW"] = false;
		animSpeeds["IDLE"] = 1.0f;
		animSpeeds["ATTACK"] = 1.0f;
		animSpeeds["WALK"] = 1.0f;
		animSpeeds["PARRY"] = 1.0f;
		animSpeeds["HURT"] = 1.0f;
		animSpeeds["DODGE"] = 1.0f;
		animSpeeds["THROW"] = 1.0f;
		animDurations["IDLE"] = 0.15f;
		animDurations["ATTACK"] = 0.1f;
		animDurations["WALK"] = 0.15f;
		animDurations["PARRY"] = 0.1f;
		animDurations["HURT"] = 0.05f;
		animDurations["DODGE"] = 0.05f;
		animDurations["THROW"] = 0.1f;
	}

	void AnimStateMachine::ResetFlags() {
		animSwitch["IDLE"] = false;
		animSwitch["ATTACK"] = false;
		animSwitch["WALK"] = false;
		animSwitch["PARRY"] = false;
		animSwitch["HURT"] = false;
		animSwitch["DODGE"] = false;
		animSwitch["THROW"] = false;
	}

	void AnimStateMachine::TransferAnims(UserResourceHandle<ResourceAnimation> inAnim[7]) {
		animations[IDLE] = inAnim[IDLE];
		animations[ATTACK] = inAnim[ATTACK];
		animations[WALK] = inAnim[WALK];
		animations[PARRY] = inAnim[PARRY];
		animations[HURT] = inAnim[HURT];
		animations[DODGE] = inAnim[DODGE];
		animations[THROW] = inAnim[THROW];
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
				animComp->TransitionTo(animSM->animations[IDLE].GetHash(),animSM->animDurations["IDLE"]);
				//animComp->animHandleA = 1058861583856284945;
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = animSM->animSpeeds["IDLE"];
				//std::cout << "Set Idle Animation" << std::endl;
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
				animComp->TransitionTo(animSM->animations[WALK].GetHash(), animSM->animDurations["WALK"]);
				//animComp->animHandleA = 2370177633938117279;
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = animSM->animSpeeds["WALK"];
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
				animComp->TransitionTo(animSM->animations[ATTACK].GetHash(), animSM->animDurations["ATTACK"]);
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = animSM->animSpeeds["ATTACK"];
			}
		}
	}

	void AttackActivity::OnUpdate(sm::StateMachine* sm)
	{
		// Optionally implement any per-frame logic for the Attack activity here
		sm::AnimStateMachine* animSM = CastSM(sm);
		float attackDelay = animSM->attackDelay;
		if (attackDelay > 0.f) {
			animSM->attackDelay -= GameTime::Dt();
			if (animSM->attackDelay <= 0.f) {
				animSM->attackDelay = 0.f;
				animSM->attack = false;
			}
			else {
				animSM->attack = true;
				//std::cout << "stay attacking" << std::endl;
			}
		}
	}

	void HurtActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[HURT].GetHash())
		{
			animComp->TransitionTo(animSM->animations[HURT].GetHash(), animSM->animDurations["HURT"]);
			animComp->isPlaying = true;
			animComp->loop = false; // Usually hurt doesn't loop
			animComp->speed = animSM->animSpeeds["HURT"];
		}
	}

	void DodgeActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[DODGE].GetHash())
		{
			animComp->TransitionTo(animSM->animations[DODGE].GetHash(), animSM->animDurations["DODGE"]);
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = animSM->animSpeeds["DODGE"];
		}
	}

	void ParryActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[PARRY].GetHash())
		{
			animComp->TransitionTo(animSM->animations[PARRY].GetHash(), animSM->animDurations["PARRY"]);
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = animSM->animSpeeds["PARRY"];
			std::cout << "Set Parry Animation" << std::endl;
		}
	}

	void ThrowActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		if (animComp && animSM->animations[THROW].GetHash())
		{
			animComp->TransitionTo(animSM->animations[THROW].GetHash(), animSM->animDurations["THROW"]);
			animComp->isPlaying = true;
			animComp->loop = false;
			animComp->speed = animSM->animSpeeds["THROW"];
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
		if (animSM->idle && !animSM->attack) {
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