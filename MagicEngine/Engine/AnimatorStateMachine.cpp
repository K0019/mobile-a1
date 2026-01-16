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
				animComp->TransitionTo(animSM->animations[IDLE].GetHash());
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
				animComp->TransitionTo(animSM->animations[WALK].GetHash());
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

	void RunActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		ecs::EntityHandle entity = animSM->GetEntity();
		AnimationComponent* animComp = entity->GetComp<AnimationComponent>();
		std::cout << "Entered Run Activity" << std::endl;
		if (animComp)
		{
			// TODO: Set your actual run animation hash
			if (animSM->animations[WALK].GetHash()) {
				animComp->TransitionTo(animSM->animations[WALK].GetHash());
				//animComp->animHandleA = 2370177633938117279;
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = 1.0f;
				std::cout << "Set Run Animation" << std::endl;
			}
		}
	}

	void AttackActivity::OnEnter(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		AnimationComponent* animComp = animSM->GetEntity()->GetComp<AnimationComponent>();
		std::cout << "Entered Attack Activity" << std::endl;
		if (animComp)
		{
			// TODO: Set your actual run animation hash
			if (animSM->animations[ATTACK].GetHash()) {
				animComp->TransitionTo(animSM->animations[ATTACK].GetHash());
				animComp->isPlaying = true;
				animComp->loop = true;
				animComp->speed = 1.0f;
				std::cout << "Set Run Animation" << std::endl;
			}
		}
	}

	//void AttackActivity::OnUpdate(sm::StateMachine* sm)
	//{
	//	// Optionally implement any per-frame logic for the Idle activity here
	//	sm::AnimStateMachine* animSM = CastSM(sm);

	//}

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

			return false;
		}
		//return animSM->walking;//speed > 0.1f && speed < 5.0f;
	}

	ToRunTransition::ToRunTransition()
		: sm::AnimTransitionBase<ToRunTransition>(SET_NEXT_STATE(RunState))
	{
	}

	bool ToRunTransition::Decide(sm::StateMachine* sm)
	{
		sm::AnimStateMachine* animSM = CastSM(sm);
		Vec3 velo = GetEntityVelo(animSM->GetEntity());
		return false;
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
			return false;
		}
		//return animSM->attack;
	}


	//======================================================================
	// STATE DEFINITIONS
	//======================================================================

	IdleState::IdleState()
		: sm::State(
			{ new IdleActivity() },
			{ new ToWalkTransition(), new ToAttackTransition()}
		)
	{
	}

	WalkState::WalkState()
		: sm::State(
			{ new WalkActivity() },
			{ new ToIdleTransition(), new ToAttackTransition()}
		)
	{
	}

	RunState::RunState()
		: sm::State(
			{ new RunActivity() },
			{ new ToIdleTransition(), new ToWalkTransition() }
		)
	{
	}

	AttackState::AttackState()
		: sm::State(
			{ new AttackActivity() },
			{ new ToIdleTransition(), new ToWalkTransition()}
		)
	{
	}

	//======================================================================
	// USAGE EXAMPLE
	//======================================================================

	/*
	// In your character component or system:
	class CharacterController
	{
	public:
		void Initialize()
		{
			// Create the animation state machine starting in idle state
			animStateMachine = new sm::AnimStateMachine(new IdleState());
		}

		void Update(ecs::EntityHandle entity)
		{
			// Update the state machine with the character entity
			animStateMachine->Update(entity);
		}

		~CharacterController()
		{
			delete animStateMachine;
		}

	private:
		sm::AnimStateMachine* animStateMachine;
	};
	*/
}