/******************************************************************************/
/*!
\file   Tween.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Regular Tween class inheriting from TweenBase. For usage with TweenManager
  singleton instance.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Tween.h"

template <typename V, typename U, typename Object>
Tween<V, U, Object>::Tween(
	Object& obj,				// Reference to object
	void (Object::* setter)(V),	// Function pointer to object's setter
	U const& vStart,			// Start value
	U const& vEnd,				// End value
	float duration,				// Duration of tween
	TT type)					// Type of tween
	: obj		{ obj }
	, setter	{ setter }
	, vStart	{ vStart }
	, vDelta	{ vEnd - vStart }
	, duration	{ duration }
	, elapsed	{ 0.0f }
	, active	{ true }
	, type		{ type }
{
}

template <typename V, typename U, typename Object>
bool Tween<V, U, Object>::IsActive() const
{
	return active;
}

template <typename V, typename U, typename Object>
void Tween<V, U, Object>::Update(float dt)
{
	if (!active)
	{
		return;
	}
	elapsed += dt; // Count up elapsed time

	if (elapsed > duration)
	{
		active = false;
		elapsed = duration; // So that we have one final update
	}

	// t value ranges from 0 to 1 depending on elapsed
	float t = elapsed / duration;

	Interpolate(t, type);

	(obj.*setter)(vStart + t * vDelta);
}
