/******************************************************************************/
/*!
\file   EnemyCharacter.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   10/26/2025

\author Matthew Chan Shao Jie (100%)
\par    email: m.chan\@digipen.edu
\par    DigiPen login: m.chan

\brief
	EnemyCharacter is an ECS component-system pair which controls enemy movement.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/GameCameraController.h"
#include "Physics/Physics.h"
#include "Engine/Input.h"
#include "Editor/Containers/GUICollection.h"

EnemyComponent::EnemyComponent()
	: playerReference{nullptr}
	, attackCollider{nullptr}
	, combatRange{}
	, windUpTime{}
	, attackTime{}
	, attackCoolDownTime{}
	, currAttackCoolDown{std::numeric_limits<float>::max()}
	, followThroughTime{}
{
}

void EnemyComponent::Serialize(Serializer& writer) const
{
	ISerializeable::Serialize(writer);
	writer.Serialize("playerReference", playerReference);
	writer.Serialize("attackCollider", attackCollider);
}

void EnemyComponent::Deserialize(Deserializer& reader)
{
	ISerializeable::Deserialize(reader);
	reader.Deserialize("playerReference", &playerReference);
	reader.Deserialize("attackCollider", &attackCollider);
}

void EnemyComponent::EditorDraw()
{
	gui::VarDefault("Combat Range", &combatRange);
	gui::VarDefault("WindUp Time", &windUpTime);
	gui::VarDefault("Attack Time", &attackTime);
	gui::VarDefault("Attack CoolDownTime", &attackCoolDownTime);
	gui::VarDefault("Follow Through Time", &followThroughTime);
	playerReference.EditorDraw("Player");
	attackCollider.EditorDraw("Attack Collider");
}

EnemyMovementComponentSystem::EnemyMovementComponentSystem()
	: System_Internal{ &EnemyMovementComponentSystem::UpdateEnemyMovementComponent }

{
}

void EnemyMovementComponentSystem::UpdateEnemyMovementComponent(EnemyComponent& comp)
{
}
