/******************************************************************************/
/*!
\file   TweenBase.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  TweenBase is the base class for template classes Tween and TweenECS. This
  allows for polymorphic behaviour, storage in std::vector and other benefits.
  Base functions include Update(), IsActive() and Interpolate().

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once

/*****************************************************************//*!
\enum TT
\brief
	Enumerator denoting various types of tweens.
*//******************************************************************/
enum TT // Tween Type
{
	LINEAR = 0,
	EASE_IN,
	EASE_OUT,
	EASE_BOTH,
	CUBIC_EASE_IN,
	CUBIC_EASE_OUT,
	CUBIC_EASE_BOTH,
	TWEEN_END
};

/*****************************************************************//*!
\class TweenBase
\brief
	Base class for tweens
*//******************************************************************/
class TweenBase
{
public:
	// Virtual destructor to ensure proper cleanup of derived classes
	virtual ~TweenBase() = default;

	// Functions for use by TweenManager and TweenComponent
	virtual void Update(float dt) = 0;
	virtual bool IsActive() const = 0;

	// Interpolation algorithms
	/*****************************************************************//*!
	\brief
		Modifies the parameter t based on the type of tween.
	\param t
		Reference to t value, which is interpolated from 0.f to 1.f.
	\param type
		Type of tween
	*//******************************************************************/
	void Interpolate(float& t, TT type);
};
