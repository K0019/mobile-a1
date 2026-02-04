/******************************************************************************/
/*!
\file   Character.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	CharacterMovementComponent is an ECS component-system pair which controls character movement.

All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Components/NameComponent.h"
#include "Game/Character.h"
#include "Game/Delusion.h"
#include "Physics/Physics.h"
#include "Physics/Collision.h"
#include "Engine/Input.h"
#include "Graphics/AnimationComponent.h"
#include "Editor/Containers/GUICollection.h"
#include "Engine/Audio.h"
#include "Managers\AudioManager.h"
#include "Graphics/BoneAttachment.h"
#include "Engine/BehaviorTree/BehaviourTree.h"
#include "Graphics/AnimatorComponent.h"

#define X(type, name) name,
char const* animNames[] =
{
	ANIMATIONS
};
#undef X

CharacterMovementComponent::CharacterMovementComponent()
	: center{}
	, radius{1.f}
	, height{1.f}
	, movementVector{ 0.0f,0.0f }
	, hitDebugObject{ nullptr }
	, moveSpeed{ 0.0f }
	, rotateSpeed{ 0.0f }
	, stunTimePerHit{ 0.0f }
	, groundFriction{ 0.0f }
	, dodgeCooldown{ 0.0f }
	, dodgeDuration{ 0.0f }
	, dodgeSpeed{ 0.0f }
	, currentDodgeCooldown{ 0.0f }
	, heldItem{ nullptr }
	, currentStunTime{ 0.0f }
	, currentDodgeTime{ 0.0f }
	, speedMultiplier{ 1.0f }
	, throwPower{0.0f}
	, parryTime{}
	, parryCoolDownTime{}
	, parryDelusion{}
	, currParryCoolDown{}
	, currParryTime{}

{
}

void CharacterMovementComponent::OnCreation()
{
	joltCharRef = ST<physics::JoltPhysics>::Get()->CreateCharacterBody(ecs::GetEntity(this)->GetHash());
}

void CharacterMovementComponent::OnAttached()
{
	joltCharRef->SetShapeOffset(JPH::Vec3{ center.x, center.y, center.z });
	ST<physics::JoltPhysics>::Get()->SetCharacterHeight(joltCharRef, height);
	ST<physics::JoltPhysics>::Get()->SetCharacterRadius(joltCharRef, radius);
}

void CharacterMovementComponent::OnDetached()
{
	JPH::CharacterContactListener* listener = joltCharRef->GetListener();
	if (listener != nullptr)
	{
		delete listener;
		joltCharRef->SetListener(nullptr);
	}
}

const Vec2 CharacterMovementComponent::GetMovementVector()
{
	return movementVector;
}

bool CharacterMovementComponent::Dodge(Vec2 vector)
{
	// Don't allow dodging if still on cooldown
	if (currentDodgeCooldown > 0.0f)
		return false;

	if (vector.LengthSqr() == 0.0f)
	{
		auto characterEntity = ecs::GetEntity(this);
		Transform& characterTransform = characterEntity->GetTransform();
		Vec3 rotation = characterTransform.GetWorldRotation();


		vector = Vec2(sin(math::ToRadians(rotation.y)), cos(math::ToRadians(rotation.y)));
	}

	/// jk
	//// Don't dodge nowhere
	//if (vector.LengthSqr() == 0.0f)
	//	return false;

	SetMovementVector(vector.Normalized());
	currentDodgeTime = dodgeDuration;
	currentDodgeCooldown = dodgeCooldown;

	// Play Audio
	ST<AudioManager>::Get()->PlaySound3D("dodge " + std::to_string(randomRange<int>(1, 3)), false, ecs::GetEntity(this)->GetTransform().GetWorldPosition(),AudioType::END,std::pair<float,float>{2.0f,50.0f}, 0.6f);

	return true;
}

void CharacterMovementComponent::SetMovementVector(Vec2 vector)
{
	// Can't change movement direction if dodging
	if (currentDodgeTime > 0.0f)
		return;

	movementVector = vector;
}

void CharacterMovementComponent::RotateTowards(Vec2 vector)
{
	auto characterEntity = ecs::GetEntity(this);
	Transform& characterTransform = characterEntity->GetTransform();

	Vec3 currentRotation = characterTransform.GetWorldRotation();

	float targetAngle = math::ToDegrees(atan2(-vector.y, vector.x))+90;
	float newAngle = math::MoveTowardsAngle(currentRotation.y, targetAngle, rotateSpeed * GameTime::Dt());
	currentRotation.y = newAngle;

	characterTransform.SetWorldRotation(currentRotation);
}

void CharacterMovementComponent::DropItem()
{
	// Sanity check!
	if (heldItem == nullptr)
		return;

	// Physics Comp related
	if (auto physicsComp{ heldItem->GetComp<physics::PhysicsComp>() })
	{
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, true);
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::USE_GRAVITY, true);
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::IS_KINEMATIC, false);
	}
	if (auto colliderComp{ heldItem->GetComp<physics::BoxColliderComp>() })
	{
		colliderComp->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, true);
	}

	heldItem->GetComp<GrabbableItemComponent>()->isHeld = false;

	// Transform related
	heldItem->GetTransform().SetParent(nullptr);

	if (auto boneAttachComp{ heldItem->GetComp<BoneAttachment>() })
	{
		boneAttachComp->targetEntity = nullptr;
	}


	heldItem = nullptr;
}

void CharacterMovementComponent::Throw(Vec3 direction)
{
	if (heldItem == nullptr)
		return;

	auto tmpItem = heldItem;
	DropItem();

	tmpItem->GetComp<physics::PhysicsComp>()->AddImpulse(direction, throwPower);
}

void CharacterMovementComponent::GrabItem(ecs::CompHandle<GrabbableItemComponent> item)
{
	// Sanity check!
	if (item == nullptr)
		return;

	item->isHeld = true;
	item->owner = ecs::GetEntity(this);
	heldItem = ecs::GetEntity(item);

	// Physics Comp related
	if(auto physicsComp{ heldItem->GetComp<physics::PhysicsComp>() })
	{
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::ENABLED, false);
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::USE_GRAVITY, false);
		physicsComp->SetFlag(physics::PHYSICS_COMP_FLAG::IS_KINEMATIC, false);
	}
	if (auto colliderComp{ heldItem->GetComp<physics::BoxColliderComp>() })
	{
		colliderComp->SetFlag(physics::COLLIDER_COMP_FLAG::ENABLED, false);
	}

	// Play Audio
	ST<AudioManager>::Get()->PlaySound3D("weapon pickup "+std::to_string(randomRange<int>(1,4)), false, ecs::GetEntity(this)->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);
}

void CharacterMovementComponent::LightAttack()
{
	// Defer to animation fsm
	ecs::GetEntity(this)->GetComp<AnimatorComponent>()->GetStateMachine()->blackboard["inputLightAttack"] = true;
}

void CharacterMovementComponent::HeavyAttack()
{
	ecs::GetEntity(this)->GetComp<AnimatorComponent>()->GetStateMachine()->blackboard["inputHeavyAttack"] = true;
}

bool CharacterMovementComponent::IsAttacking() const
{
	auto animFSM{ ecs::GetEntity(this)->GetComp<AnimatorComponent>()->GetStateMachine() };
	return animFSM->GetBlackboardVal<bool>("inputLightAttack") || animFSM->GetBlackboardVal<bool>("inputHeavyAttack");
}

bool CharacterMovementComponent::IsParrying()
{
	return currParryTime > 0.f;
}
void CharacterMovementComponent::OnParrySuccess()
{
	if (auto delusionComp{ ecs::GetEntity(this)->GetComp<DelusionComponent>() })
		delusionComp->AddDelusion(parryDelusion);

	CONSOLE_LOG(LEVEL_INFO) << "Parried.";
}
void CharacterMovementComponent::Parry()
{
	if(currParryCoolDown <= 0.f)
	{
		currParryTime = parryTime;
		currParryCoolDown = parryCoolDownTime;
	}
}

bool CharacterMovementComponent::IsDodging()
{
	return currentDodgeTime > 0.0f;
}

ecs::CompHandle<GrabbableItemComponent> CharacterMovementComponent::GetHeldItem()
{
	if (heldItem)
		if (auto itemComp{ heldItem->GetComp<GrabbableItemComponent>() })
			return itemComp;
	return ecs::GetEntity(this)->GetComp<GrabbableItemComponent>();
}

void CharacterMovementComponent::Serialize(Serializer& writer) const
{
	IRegisteredComponent::Serialize(writer);
	writer.Serialize("moveSpeed", moveSpeed);
	writer.Serialize("rotateSpeed", rotateSpeed);
	writer.Serialize("stunTimePerHit", stunTimePerHit);
	writer.Serialize("groundFriction", groundFriction);

	writer.Serialize("dodgeCooldown", dodgeCooldown);
	writer.Serialize("dodgeDuration", dodgeDuration);
	writer.Serialize("dodgeSpeed", dodgeSpeed);


	writer.Serialize("hitDebugObject", hitDebugObject);
	writer.Serialize("heldItem", heldItem);

	size_t maxArraySize = ANIM_TOTAL;

	// Load all animations
	writer.StartArray("animations");
	for (size_t i = 0; i < maxArraySize; ++i)
	{
		writer.StartObject();
		writer.Serialize(animations[i]);
		writer.EndObject();
	}
	writer.EndArray();
}

void CharacterMovementComponent::Deserialize(Deserializer& reader)
{
	IRegisteredComponent::Deserialize(reader);

	reader.DeserializeVar("moveSpeed", &moveSpeed);
	reader.DeserializeVar("rotateSpeed", &rotateSpeed);
	reader.DeserializeVar("stunTimePerHit", &stunTimePerHit);
	reader.DeserializeVar("groundFriction", &groundFriction);

	reader.DeserializeVar("dodgeCooldown", &dodgeCooldown);
	reader.DeserializeVar("dodgeDuration", &dodgeDuration);
	reader.DeserializeVar("dodgeSpeed", &dodgeSpeed);

	reader.DeserializeVar("hitDebugObject", &hitDebugObject);
	reader.DeserializeVar("heldItem", &heldItem);

	// Load animations

	if (reader.PushAccess("animations"))
	{
		for (size_t i = 0; i < ANIM_TOTAL; ++i)
		{
			if (reader.PushArrayElementAccess(i))
			{
				reader.Deserialize(&animations[i]);
				reader.PopAccess();
			}
		}
		reader.PopAccess();
	}
}

void CharacterMovementComponent::SetSpeedMultiplier(float mult)
{
	speedMultiplier = mult;
}

void CharacterMovementComponent::ResetSpeedMultiplier()
{
	speedMultiplier = 1.0f;
}

void CharacterMovementComponent::SetCenter(const Vec3& vec)
{
	center = vec;
	joltCharRef->SetShapeOffset(JPH::Vec3{ vec.x, vec.y, vec.z });
}

void CharacterMovementComponent::EditorDraw()
{
	if (gui::VarDrag("Center", &center, 1.0f, Vec3{}, Vec3{}, "%.1f"))
		joltCharRef->SetShapeOffset(JPH::Vec3{ center.x, center.y, center.z });

	if (gui::VarInput("Radius", &radius))
		ST<physics::JoltPhysics>::Get()->SetCharacterRadius(joltCharRef, radius);
	if (gui::VarInput("Height", &height))
		ST<physics::JoltPhysics>::Get()->SetCharacterHeight(joltCharRef, height);
	
	gui::VarInput("Move Speed", &moveSpeed);
	gui::VarInput("Rotation Speed", &rotateSpeed);
	gui::VarInput("Stun Time Per Hit", &stunTimePerHit);
	gui::VarInput("Ground Friction", &groundFriction);

	gui::VarInput("Dodge Cooldown", &dodgeCooldown);
	gui::VarInput("Dodge Duration", &dodgeDuration);
	gui::VarInput("Dodge Speed", &dodgeSpeed);
	gui::VarInput("Throw Power", &throwPower);

	gui::VarInput("Parry Time Period", &parryTime);
	gui::VarInput("Parry Cool Down time", &parryCoolDownTime);
	gui::VarInput("Parry Delusion", &parryDelusion);

	hitDebugObject.EditorDraw("Hit Debug Object");
	heldItem.EditorDraw("Held Item");

	// Animation input
	for(uint32_t animIndex = 0;animIndex< ANIM_TOTAL;++animIndex)
	{
		const std::string* clip1Name{ ST<MagicResourceManager>::Get()->Editor_GetName(animations[animIndex].GetHash()) };
		gui::TextUnformatted(std::string(animNames[animIndex]));
		gui::SameLine();
		gui::TextBoxReadOnly(std::string("##AnimClip"+std::to_string(animIndex)).c_str(), clip1Name ? clip1Name->c_str() : "");
		gui::PayloadTarget<size_t>("ANIMATION_HASH", [&](size_t hash) -> void {
			animations[animIndex] = hash;
		});
	}
}

CharacterMovementComponentSystem::CharacterMovementComponentSystem()
	: System_Internal{ &CharacterMovementComponentSystem::UpdateCharacterMovementComponent }
{
}



void CharacterMovementComponentSystem::UpdateCharacterMovementComponent(CharacterMovementComponent& comp)
{
	auto characterEntity = ecs::GetEntity(&comp);
	Transform& characterTransform = characterEntity->GetTransform();

	// Get the animation component
	ecs::CompHandle<AnimationComponent> animComp = characterEntity->GetComp<AnimationComponent>();
	ecs::CompHandle<AnimatorComponent> animatorComp = characterEntity->GetComp<AnimatorComponent>();
	if (!animatorComp)
		animatorComp = characterEntity->AddComp<AnimatorComponent>(AnimatorComponent{ new sm::AnimStateMachine(new sm::IdleState()) });


	// Update held item
	if (ecs::EntityHandle attackItem{ comp.heldItem })
	{
		// Transform related
		attackItem->GetTransform().SetParent(characterTransform);
		attackItem->GetComp<GrabbableItemComponent>()->owner = characterEntity;
		attackItem->GetComp<GrabbableItemComponent>()->isHeld = true;

		if (auto boneAttachComp{ attackItem->GetComp<BoneAttachment>() })
		{
			boneAttachComp->targetEntity = characterEntity;
			boneAttachComp->boneName = "J_Bip_R_Hand";
		}
	}

	// Update animation FSM
	animatorComp->GetStateMachine()->Update(characterEntity);

	// Perform stun check
	ecs::CompHandle<physics::PhysicsComp> physicsComp = characterEntity->GetComp<physics::PhysicsComp>();
	Vec3 currVel{};
	if (physicsComp->GetIsKinematic())
	{
		JPH::Vec3 joltCurrVel = comp.joltCharRef->GetLinearVelocity();
		currVel = Vec3{ joltCurrVel.GetX(), joltCurrVel.GetY(), joltCurrVel.GetZ() };
		currVel = util::WalkTo(currVel, Vec3{}, Vec3{ 4.0f * GameTime::Dt() }); // Apply friction
		comp.joltCharRef->SetLinearVelocity(JPH::Vec3{ currVel.x, currVel.y, currVel.z });
	}
	else
		currVel = physicsComp->GetLinearVelocity();

	// Get inputs
	Vec2 movement = comp.GetMovementVector();
	if (comp.currentStunTime > 0.0f)
	{
		comp.currentStunTime -= GameTime::Dt();

		animatorComp->GetStateMachine()->blackboard["inputHurt"] = true;

		//if (animComp->animHandleA.GetHash() != comp.animations[HURT].GetHash())
		//{
		//	animComp->TransitionTo(comp.animations[HURT].GetHash(), 0.1f);

		//	//comp.animations[]
		//}
		//animComp->timeA = comp.currentStunTime / comp.stunTimePerHit;
		comp.currentDodgeTime = 0.0f;

		if (!physicsComp->GetIsKinematic())
			physicsComp->SetLinearVelocity(Vec3{ currVel.x, currVel.y, currVel.y });
		else
		{
			Vec3 currPos{ characterTransform.GetWorldPosition() };
			comp.joltCharRef->SetPosition(JPH::Vec3{ currPos.x, currPos.y, currPos.z });
			ST<physics::JoltPhysics>::Get()->UpdateCharacterBody(characterEntity, comp.joltCharRef, Vec3{currVel.x, 0.f, currVel.z});
			JPH::Vec3 joltPos{ comp.joltCharRef->GetPosition() };
			characterTransform.SetWorldPosition(Vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ()));
		}
		return;
	}

	if (comp.IsParrying())
	{
		animatorComp->GetStateMachine()->blackboard["inputParry"] = true;

		//if (auto clip{ animComp->GetAnimationClipA() })
		//{
		//	float duration = animComp->GetClipDuration(clip);
		//	animComp->timeA = duration * (1.0f - (comp.currParryTime / comp.parryTime));
		//}
		//else
		//{
		//	animComp->timeA = 0.0f;
		//}
		//comp.currParryTime -= GameTime::Dt();
		return;
	}
	comp.currParryCoolDown -= GameTime::Dt();

	// Get inputs
	if (movement.LengthSqr() > 1.0f)
		movement = movement.Normalized();
	animatorComp->GetStateMachine()->blackboard["inputMovement"] = movement;

	// Apply friction
	//Vec3 drag{ -currVel.x,0.0f,-currVel.z };
	//float groundSpeed = drag.Length();
	//if (groundSpeed <= comp.groundFriction)
	//{
	//	physicsComp->AddLinearVelocity(drag);
	//}
	//else
	//{
	//	physicsComp->AddLinearVelocity(drag.Normalized() * comp.groundFriction * groundSpeed);
	//}

	// Apply input movement
	Vec3 moveDir = Vec3{ movement.x , (physicsComp->GetIsKinematic() ? 0.f : currVel.y), movement.y };

	// If dodging, move faster
	if (comp.currentDodgeTime > 0.0f)
	{
		comp.currentDodgeTime -= GameTime::Dt();
		moveDir *= comp.dodgeSpeed;

		animatorComp->GetStateMachine()->blackboard["inputDodge"] = true;
	}
	else
	{
		moveDir *= comp.moveSpeed * comp.speedMultiplier;
	}

	ST<physics::JoltPhysics>::Get()->UpdateCharacterBody(characterEntity, comp.joltCharRef, Vec3{ moveDir.x, currVel.y, moveDir.z });

	comp.currentDodgeCooldown -= GameTime::Dt();

	if (movement.LengthSqr() > 0.0f)
		comp.RotateTowards(movement);

	// Handle parry time/cooldown
	if (comp.currParryTime > 0.f)
		comp.currParryTime -= GameTime::Dt();
	if (comp.currParryCoolDown > 0.f)
		comp.currParryCoolDown -= GameTime::Dt();

	// Check whether to apply an attack this frame
	int attackMoveIndex{ animatorComp->GetStateMachine()->GetBlackboardVal<int>("outputApplyHitMove") };
	if (attackMoveIndex >= 0)
	{
		ApplyAttack(static_cast<size_t>(attackMoveIndex), characterTransform, comp);
		animatorComp->GetStateMachine()->blackboard["outputApplyHitMove"] = -1;
	}

	comp.SetMovementVector(Vec2{ 0.f, 0.f });
}

bool CharacterMovementComponentSystem::PreRun()
{
	physics::MyCharacterContactListener::ClearContactPair();
	return true;
}

void CharacterMovementComponentSystem::PostRun()
{
	physics::MyCharacterContactListener::CallContactFunc();
}

void CharacterMovementComponentSystem::ApplyAttack(size_t moveIndex, const Transform& transform, CharacterMovementComponent& charComp)
{
	auto heldItem{ charComp.GetHeldItem() };
	auto weaponInfo{ heldItem->weaponInfo.GetResource() };
	if (!weaponInfo || weaponInfo->moves.size() < moveIndex)
	{
		CONSOLE_LOG(LEVEL_ERROR) << "Character doesn't have WeaponInfo or WeaponInfo doesn't have a move at index " << moveIndex << ", unable to apply attack hit logic";
		return;
	}

	const auto& weaponMove{ weaponInfo->moves[moveIndex] };

	// Calculate the hitbox position based on current player's position and rotation
	float yRot{ math::ToRadians(transform.GetWorldRotation().y) };
	Vec3 rotatedOffset{ glm::rotateY(weaponMove.hitboxOffset, yRot) };
	Vec3 hitboxOrigin{ transform.GetWorldPosition() + rotatedOffset };
	hitboxOrigin.y += 0.8f; // Add a bit of moving of the hitbox up from the floor automatically, if not needed can compensate in moves struct

	/*auto hitDebugObject = thisComp->hitDebugObject;
	if (hitDebugObject != nullptr)
	{
		hitDebugObject->GetTransform().SetWorldPosition(startPoint);
		hitDebugObject->GetTransform().SetWorldRotation(Vec3(0.0f, math::ToDegrees(atan2(direction.x, direction.z)), 0.0f));
		hitDebugObject->GetTransform().SetWorldScale(attackItem->GetComp<GrabbableItemComponent>()->attackBox);
	}*/

	charComp.GetHeldItem()->Attack(hitboxOrigin, weaponMove.hitboxExtents);
}
