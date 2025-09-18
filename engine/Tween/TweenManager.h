/******************************************************************************/
/*!
\file   TweenManager.h
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
#pragma once
#include "Tween.h"
#include "TweenECS.h"

template<typename T>
class ST; // Forward declare singleton template

/*****************************************************************//*!
\class TweenManager
\brief
	The primary class for tween management. Also directly manages the
	list of general object tweens.
*//******************************************************************/
class TweenManager
{
public:
	friend class ST<TweenManager>;

	~TweenManager(); // Public destructor

	// For general tweens
	/*****************************************************************//*!
	\brief
		Starts tweens for general objects.
	\param obj
		Reference to the object.
	\tparam setter
		Function pointer to the object's setter.
	\tparam vStart
		Start value.
	\tparam vEnd
		End value
	\param duration
		Duration of interpolation in seconds.
	*//******************************************************************/
	template<typename V, typename U, typename Object>
	void StartTween(
		Object& obj, 
		void (Object::* setter)(V), 
		U const& vStart, 
		U const& vEnd, 
		float duration,
		TT type = TT::CUBIC_EASE_IN);

	// For entity tweens
	/*****************************************************************//*!
	\brief
		Starts tweens for ECS entities. This function can safely
		interpolate values within ECS entities as it attaches a
		TweenComponent to the entity. When the entity dies, the
		TweenComponent dies as well so it never references garbage.
		Additionally, the separation of V and U template parameters allows
		for pass-by-value and pass-by-reference syntax in the setter.
	\param entity
		EntityHandle, a.k.a pointer to the entity.
	\tparam setter
		Function pointer to the object's setter.
	\tparam vStart
		Start value.
	\tparam vEnd
		End value
	\param duration
		Duration of interpolation in seconds.
	\param type
		Type of interpolation.
	*//******************************************************************/
	template <typename V, typename U, typename Object>
	void StartTween(
		ecs::EntityHandle entity,	// Handle to the entity
		void (Object::* setter)(V),	// Function pointer to object's setter
		U const& vStart,			// Start value
		U const& vEnd,				// End value
		float duration,				// Duration of tween
		TT type = TT::LINEAR);		// Type of tween

	/*****************************************************************//*!
	\brief
		Updates general tweens.
	\param dt
		Time elapsed since the previous frame.
	*//******************************************************************/
	void Update(float dt);

	/*****************************************************************//*!
	\brief
		Cleans the tweens vector.
	*//******************************************************************/
	void Clean();

private:
	/*****************************************************************//*!
	\brief
		Mostly empty constructor.
	*//******************************************************************/
	TweenManager(); // Private constructor

	std::vector<std::unique_ptr<TweenBase>> tweens; // Non-entity tweens
};

#include "TweenManager.ipp"
