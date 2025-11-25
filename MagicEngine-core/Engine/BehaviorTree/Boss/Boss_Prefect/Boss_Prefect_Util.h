#pragma once
#include "Game/EnemyCharacter.h"
#include "Game/Character.h"
#include "Game/Health.h"
// Utilities for Boss_Prefect


class Boss_Prefect_Util
{
public:
	static float minFollowDistance;
	static float maxFollowDistance;

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
};

// Init values here, square them early to save some processing
float Boss_Prefect_Util::minFollowDistance = 2.0f * 2.0f;
float Boss_Prefect_Util::maxFollowDistance = 4.0f * 4.0f;

