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
	, isAttacking{ false }
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

bool CharacterMovementComponent::Attack()
{
	// If already in attack animation, skip
	if (isAttacking)
		return false;
	isAttacking = true;

	ecs::EntityHandle attackItem{ heldItem };
	// If not holding an item, we fallback to the character's entity itself
	if (attackItem == nullptr && ecs::GetEntity(this)->GetComp<GrabbableItemComponent>())
	{
		attackItem = ecs::GetEntity(this);
	}

	// If the entity doesn't have a GI comp, then it has no unarmed attack.
	else if (attackItem == nullptr)
		return false;

	// Audio plays here
	//if (auto audioSourceComp{ ecs::GetEntity(this)->GetComp<AudioSourceComponent>() })
	//{
	//	//audioSourceComp->Set
	//}

	ecs::EntityHandle thisEntity = ecs::GetEntity(this);

	// Get the animation component
	ecs::CompHandle<AnimationComponent> animComp = thisEntity->GetComp<AnimationComponent>();

	// Attempt to use animation pulled from the item, if nonexistent then use the fallback anim on the Character
	size_t attackAnimHash = attackItem->GetComp<GrabbableItemComponent>()->lightAttackAnimation.GetHash();
	if (attackAnimHash == 0)
		attackAnimHash = animations[ATTACK].GetHash();
	
	// Use transition for attack animation (quick transition)
	animComp->TransitionTo(attackAnimHash, 0.1f);
	//animComp->timeA = 0.0f;



    std::string tmpName;
	auto grabbableComp = attackItem->GetComp<GrabbableItemComponent>();
    if (grabbableComp->audioStartIndex > grabbableComp->audioEndIndex + 1)
        tmpName = grabbableComp->audioName + std::to_string(randomRange(grabbableComp->audioEndIndex + 1, grabbableComp->audioStartIndex));
	else
        tmpName = grabbableComp->audioName + std::to_string(randomRange(grabbableComp->audioStartIndex, grabbableComp->audioEndIndex + 1));

	//if (randomRange(0, 2) == 0)
	ST<AudioManager>::Get()->PlaySound3D(tmpName, false, ecs::GetEntity(this)->GetTransform().GetWorldPosition(), AudioType::END, std::pair<float, float>{2.0f, 50.0f}, 0.6f);

	// Handle next attack delay
	float nextAttackDelay = grabbableComp->attackDelay;
	if (auto clip{ animComp->GetAnimationClipA() })
	{
		nextAttackDelay = animComp->GetClipDuration(clip);
	}

	ST<Scheduler>::Get()->Add(grabbableComp->attackDelay, [attackItem, thisEntity]() {
		if (!ecs::IsEntityHandleValid(thisEntity))
			return;
		ecs::CompHandle<CharacterMovementComponent> thisComp{ thisEntity->GetComp<CharacterMovementComponent>() };
		if (!thisComp)
			return;

		// If the attack animation was cancelled, we cancel this task as well
		if (!thisComp->isAttacking)
			return;

		auto grabbableComp = attackItem->GetComp<GrabbableItemComponent>();

		// Hard-code a simple start point etc for now
		Vec3 rotation = thisEntity->GetTransform().GetWorldRotation();
		Vec3 direction(sin(math::ToRadians(rotation.y)), 0, cos(math::ToRadians(rotation.y)));
		Vec3 startPoint = thisEntity->GetTransform().GetWorldPosition() + direction * 0.5f * grabbableComp->attackBox.z;
		startPoint.y += 0.8f;

		auto hitDebugObject = thisComp->hitDebugObject;
		if (hitDebugObject != nullptr)
		{
			hitDebugObject->GetTransform().SetWorldPosition(startPoint);
			hitDebugObject->GetTransform().SetWorldRotation(Vec3(0.0f, math::ToDegrees(atan2(direction.x, direction.z)), 0.0f));
			hitDebugObject->GetTransform().SetWorldScale(attackItem->GetComp<GrabbableItemComponent>()->attackBox);
		}

		// Call Attack from the GrabbableItem component
		attackItem->GetComp<GrabbableItemComponent>()->Attack(startPoint, direction); 
	});
	ST<Scheduler>::Get()->Add(nextAttackDelay, [attackItem, thisEntity]() {
		if (!ecs::IsEntityHandleValid(thisEntity))
			return;
		ecs::CompHandle<CharacterMovementComponent> thisComp{ thisEntity->GetComp<CharacterMovementComponent>() };
		if (!thisComp)
			return;

		thisComp->isAttacking = false;
	});

	return true;
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

	// TODO: This is copied from Transform.cpp. Should unify both implementations in GUICollection in the future.
// Helper function for drawing the controls
	const auto DrawVec3Control = [](const char* label, Vec3* values, float columnWidth, float speed, const char* format) -> bool {
		bool modified = false;
		if (gui::Table table{ label, 4, true, gui::FLAG_TABLE::HIDE_HEADER })
		{
			table.AddColumnHeader("##", gui::FLAG_TABLE_COLUMN::WIDTH_FIXED, columnWidth); // Set first column as fixed width. The rest will be stretch columns.
			table.SubmitColumnHeaders();

			gui::TextUnformatted(label);
			table.NextColumn();

			const auto DrawFloatComponent{ [&modified, speed, format](const char* text, const char* label, float* value, const gui::Vec4& textColor) -> void {
				{
					gui::SetStyleColor styleColText{ gui::FLAG_STYLE_COLOR::TEXT, textColor };
					gui::TextUnformatted(text);
				}
				gui::SameLine();
				gui::SetNextItemWidth(gui::GetAvailableContentRegion().x);
				modified |= gui::VarDrag(label, value, speed, 0.0f, 0.0f, format);
			} };

			DrawFloatComponent("X", "##X", &values->x, gui::Vec4{ 1.0f, 0.4f, 0.4f, 1.0f });
			table.NextColumn();
			DrawFloatComponent("Y", "##Y", &values->y, gui::Vec4{ 0.4f, 1.0f, 0.4f, 1.0f });
			table.NextColumn();
			DrawFloatComponent("Z", "##Z", &values->z, gui::Vec4{ 0.4f, 0.4f, 1.0f, 1.0f });
		}
		return modified;
		};

	if (DrawVec3Control("Center", &center, 60.f, 1.f, "%.1f"))
	{
		JPH::Vec3 joltOffset{ center.x, center.y, center.z };
		joltCharRef->SetShapeOffset(joltOffset);
	}

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

	// Update held item
	ecs::EntityHandle attackItem{ comp.heldItem };
	if (attackItem)
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

	// If not holding an item, we fallback to the character's entity itself
	if (attackItem == nullptr && characterEntity->GetComp<GrabbableItemComponent>())
	{
		attackItem = characterEntity;
	}

	// Perform stun check
	ecs::CompHandle<physics::PhysicsComp> physicsComp = characterEntity->GetComp<physics::PhysicsComp>();
	Vec3 currVel{};
	if (physicsComp->GetIsKinematic())
	{
		JPH::Vec3 joltCurrVel = comp.joltCharRef->GetLinearVelocity();
		currVel = Vec3{ joltCurrVel.GetX(), joltCurrVel.GetY(), joltCurrVel.GetZ() };
	}
	else
		currVel = physicsComp->GetLinearVelocity();

	if (comp.currentStunTime > 0.0f)
	{
		comp.currentStunTime -= GameTime::Dt();

		// Can only come out of stun when on the ground
		if (math::Abs(currVel.y) > 0.01f && comp.currentStunTime < 0.0f)
			comp.currentStunTime = GameTime::Dt();

		if (animComp->animHandleA.GetHash() != comp.animations[HURT].GetHash())
		{
			animComp->TransitionTo(comp.animations[HURT].GetHash(), 0.1f);

			//comp.animations[]
		}
		animComp->timeA = comp.currentStunTime / comp.stunTimePerHit;
		comp.currentDodgeTime = 0.0f;
		return;
	}

	ecs::CompHandle<GrabbableItemComponent> itemComp = nullptr;

	if (attackItem)
		itemComp = attackItem->GetComp<GrabbableItemComponent>();

	if (comp.IsParrying())
	{
		// Get parry animation - prefer item's parry animation, fallback to character's
		size_t parryAnimHash = 0;
		if (itemComp && itemComp->parryAnimation.GetHash() != 0)
			parryAnimHash = itemComp->parryAnimation.GetHash();
		else
			parryAnimHash = comp.animations[PARRY].GetHash();

		// Transition to parry animation if not already playing it
		if (animComp->animHandleA.GetHash() != parryAnimHash)
			animComp->TransitionTo(parryAnimHash, 0.05f);

		if (auto clip{ animComp->GetAnimationClipA() })
		{
			float duration = animComp->GetClipDuration(clip);
			animComp->timeA = duration * (1.0f - (comp.currParryTime / comp.parryTime));
		}
		else
		{
			animComp->timeA = 0.0f;
		}
		comp.currParryTime -= GameTime::Dt();
		return;
	}
	comp.currParryCoolDown -= GameTime::Dt();

	// Get inputs
	Vec2 movement = comp.GetMovementVector();

	// Normalize the move vector if it's over 1.0f in length
	if (movement.LengthSqr() > 0.0f)
	{
		// Walking - only change animation if not attacking
		if (!comp.isAttacking && animComp->animHandleA.GetHash() != comp.animations[WALK].GetHash())
		{
			animComp->TransitionTo(comp.animations[WALK].GetHash(), 0.15f);
		}
	}
	else
	{
		// Idle - only change animation if not attacking
		if (!comp.isAttacking && animComp->animHandleA.GetHash() != comp.animations[IDLE].GetHash())
		{
			animComp->TransitionTo(comp.animations[IDLE].GetHash(), 0.15f);
		}
	}

	///if (comp.isAttacking)
	///{
	///	animComp->loop = false;
	///	if (animComp->timeA >= animComp->GetClipDuration(animComp->GetAnimationClipA()))
	///	{
	///		comp.isAttacking = false;
	///	}
	///}

	//animComp->loop = !comp.isAttacking;


	if (movement.LengthSqr() > 1.0f)
		movement = movement.Normalized();

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
		if (animComp->animHandleA.GetHash() != comp.animations[DODGE].GetHash())
			animComp->TransitionTo(comp.animations[DODGE].GetHash(), 0.05f);
	}
	else
	{
		moveDir *= comp.moveSpeed * comp.speedMultiplier;
	}

	if (!physicsComp->GetIsKinematic())
		physicsComp->SetLinearVelocity(Vec3{ moveDir.x, currVel.y, moveDir.z });
	else
	{
		Vec3 currPos{ characterTransform.GetWorldPosition() };
		comp.joltCharRef->SetPosition(JPH::Vec3{ currPos.x, currPos.y, currPos.z });
		ST<physics::JoltPhysics>::Get()->UpdateCharacterBody(comp.joltCharRef, moveDir);
		JPH::Vec3 joltPos{ comp.joltCharRef->GetPosition() };
		characterTransform.SetWorldPosition(Vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ()));
	}

	comp.currentDodgeCooldown -= GameTime::Dt();

	if (movement.LengthSqr() > 0.0f)
		comp.RotateTowards(movement);

	// Handle parry time/cooldown
	if (comp.currParryTime > 0.f)
		comp.currParryTime -= GameTime::Dt();
	if (comp.currParryCoolDown > 0.f)
		comp.currParryCoolDown -= GameTime::Dt();
}
