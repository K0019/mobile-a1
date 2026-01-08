#pragma once
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
#include "Engine/PrefabManager.h"
#include "Scripting/ScriptComponent.h"

// Utilities for Boss_Prefect


class Boss_Prefect_Util
{
public:
	static float minFollowDistance;
	static float maxFollowDistance;
	static float rotateSpeed;

	static Vec2 GetMovementDirection(Vec3 currentPos, Vec3 targetPos)
	{
		Vec2 outputVector{ 0.0f };
		Vec2 direction{ targetPos.x - currentPos.x,targetPos.z - currentPos.z };

		Vec2 directionNorm = direction.Normalized();
		Vec2 directionPerpendicular = { directionNorm.y,-directionNorm.x };

		// Too close, we walk away
		if (direction.LengthSqr() < minFollowDistance)
		{
			// Walk away
			outputVector = outputVector - directionNorm;
		}
		// Too far, we walk closer
		else if (direction.LengthSqr() > maxFollowDistance)
		{
			// Walk closer
			outputVector = outputVector + directionNorm;
		}

		// Add perp. for orbiting motion
		outputVector = outputVector + directionPerpendicular;

		return outputVector;
	}
	static Vec2 GetMovementTowards(Vec3 currentPos, Vec3 targetPos)
	{
		return { targetPos.x - currentPos.x,targetPos.z - currentPos.z };
	}
	static bool SpawnExplosion(ecs::EntityHandle entity, float size)
	{
		ecs::EntityHandle spawnedExplosion = ST<PrefabManager>::Get()->LoadPrefab("explosion");

		// Sanityyyyy
		if (spawnedExplosion)
		{
			// Set the size in the LUA script
			if (auto scriptComp{ spawnedExplosion->GetComp<ScriptComponent>() })
			{
				scriptComp->CallScriptFunction("setSize", size);
			}

			spawnedExplosion->GetTransform().SetWorldPosition(entity->GetTransform().GetWorldPosition());
			return true;
		}

		// This is a failsafe in case we want to check whether the prefab spawned
		return false;
	}

	static void MoveInDirection(ecs::EntityHandle entity, Vec3 direction)
	{
		entity->GetTransform().AddWorldPosition(direction * GameTime::Dt());
	}
	static void RotateTowards(ecs::EntityHandle entity, Vec2 direction)
	{
		Transform& characterTransform = entity->GetTransform();

		Vec3 currentRotation = characterTransform.GetWorldRotation();

		float targetAngle = math::ToDegrees(atan2(-direction.y, direction.x))+90;
		float newAngle = math::MoveTowardsAngle(currentRotation.y, targetAngle, rotateSpeed * GameTime::Dt());
		currentRotation.y = newAngle;

		characterTransform.SetWorldRotation(currentRotation);
	}
};


