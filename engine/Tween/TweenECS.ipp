/******************************************************************************/
/*!
\file   TweenECS.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Template function defines for TweenECS.h.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TweenECS.h"

template<typename V, typename U, typename Object>
TweenECS<V, U, Object>::TweenECS(
	ecs::EntityHandle entity, 
	void(Object::* setter)(V), 
	U const& vStart, 
	U const& vEnd, 
	float duration, 
	TT type)
	: entity	{ entity }
	, setter	{ setter }
	, vStart	{ vStart }
	, vDelta	{ vEnd - vStart }
	, duration	{ duration }
	, elapsed	{ 0.0f }
	, type		{ type }
	, active	{ true }
{
}

template<typename V, typename U, typename Object>
inline bool TweenECS<V, U, Object>::IsActive() const
{
	return active;
}

template<typename V, typename U, typename Object>
void TweenECS<V, U, Object>::Update(float dt)
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

	// Set tmp to either Transform or Component
	Object* tmp{ nullptr };

	if (typeid(Object) == typeid(Transform))
	{
		tmp = reinterpret_cast<Object*>(&entity->GetTransform());
	}
	else
	{
		tmp = entity->GetComp<Object>();
	}

	// If cannot get for any reason, return
	if (!tmp)
	{
		active = false;
		return;
	}

	// t value ranges from 0 to 1 depending on elapsed
	float t = elapsed / duration;

	// Interpolate values
	Interpolate(t, type);

	(*tmp.*setter)(vStart + t * vDelta);
}

template<typename V, typename U, typename Object>
void (Object::*TweenECS<V, U, Object>::GetSetter())(V)
{
	return setter;
}

template <typename V, typename U, typename Object>
void TweenComponent::StartTween(
	void (Object::* setter)(V),
	U const& vStart,
	U const& vEnd,
	float duration,
	TT type)
{
	// CONSOLE_LOG_EXPLICIT("ECS Tween Started!", LogLevel::LEVEL_INFO);

	// Check if other tweens are using the same function pointer and remove
	for (auto it = tweens.begin(); it != tweens.end();)
	{
		// First try to dynamic cast to the tween type
		auto *tmp = dynamic_cast<TweenECS<V, U, Object>*>(*it);

		// If dynamic cast returns a nullptr, it's not even the same type so we move on
		if (!tmp)
		{
			++it;
			continue;
		}
		
		// Else we compare function pointers
		if (tmp->GetSetter() == setter)
		{
			delete (*it);
			it = tweens.erase(it);
			continue;
		}

		++it;
	}

	// Then push the new tween
	tweens.push_back(new TweenECS<V, U, Object>(ecs::GetEntity(this), setter, vStart, vEnd, duration, type));
}
