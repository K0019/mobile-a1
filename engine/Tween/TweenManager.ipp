/******************************************************************************/
/*!
\file   TweenManager.ipp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  TweenManager is a singleton class initialised when it is first accessed and
  dies at the end of the program. It is the primary interface for starting
  tweens (interpolating values), whether it be from general objects or ECS
  entities.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "TweenManager.h"

template <typename V, typename U, typename Object>
void TweenManager::StartTween(
	Object& object,
	void (Object::*setter)(V),
	U const& vStart, 
	U const& vEnd, 
	float duration,
	TT type)
{
	// CONSOLE_LOG_EXPLICIT("Tween Started!", LogLevel::LEVEL_INFO);
	tweens.push_back(std::make_unique<Tween<V, U, Object>>(object, setter, vStart, vEnd, duration, type));
}

template<typename V, typename U, typename Object>
void TweenManager::StartTween(
	ecs::EntityHandle entity, 
	void(Object::* setter)(V), 
	U const& vStart, 
	U const& vEnd, 
	float duration, 
	TT type)
{
	if (entity == nullptr)
	{
		return;
	}

	TweenComponent* tmp = entity->GetComp<TweenComponent>();
	if (tmp == nullptr)
	{
		tmp = entity->AddCompNow(TweenComponent{});
	}
	tmp->StartTween(setter, vStart, vEnd, duration, type);
}
