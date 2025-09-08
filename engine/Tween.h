/******************************************************************************/
/*!
\file   Tween.h
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
#include "TweenBase.h"

/*****************************************************************//*!
\class Tween
\brief
	Template class inheriting from TweenBase. Stores a reference to
	an object. It is up to the user to ensure that the object remains
	valid throughout the lifetime of this tween.
*//******************************************************************/
template <typename V, typename U, typename Object>
class Tween : public TweenBase
{
public:
	/*****************************************************************//*!
	\brief
		Constructor to store values.
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
	\param type
		Type of interpolation.
	*//******************************************************************/
	Tween(
		Object& obj,				// Reference to object
		void (Object::* setter)(V),	// Function pointer to object's setter
		U const& vStart,			// Start value
		U const& vEnd,				// End value
		float duration,				// Duration of tween
		TT type);					// Type of tween

	/*****************************************************************//*!
	\brief
		Tells you if this tween is currently active.
	\return
		True if active. False otherwise.
	*//******************************************************************/
	bool IsActive() const;

	/*****************************************************************//*!
	\brief
		Interpolates a value from vStart to vStart + vDelta depending on
		the ratio of elapsed time to duration. Uses the object's setter
		function on the object reference.
	\param dt
		Time elapsed since the previous frame.
	*//******************************************************************/
	void Update(float dt);

private:
	Object& obj;
	void (Object::* setter)(V);
	U vStart;
	U vDelta;
	float duration;
	float elapsed;
	bool active;
	TT type;
};

#include "Tween.ipp"
