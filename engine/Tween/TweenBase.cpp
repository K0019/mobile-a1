/******************************************************************************/
/*!
\file   TweenBase.cpp
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
#include "TweenBase.h"

void TweenBase::Interpolate(float& t, TT type)
{
	switch (type)
	{
	case TT::LINEAR:
	{
		// Current = Start + t * Delta
		break;
	}
	case TT::EASE_IN:
	{
		// y = x^2
		t = t * t;
		break;
	}
	case TT::EASE_OUT:
	{
		// y = 1 - (1 - x)^2
		t = 1 - (1 - t) * (1 - t);
		break;
	}
	case TT::EASE_BOTH:
	{
		// y = x^2 / x^2 + (1 - x)^2
		t = (t * t) / ((t * t) + ((1 - t) * (1 - t)));
		break;
	}
	case TT::CUBIC_EASE_IN:
	{
		// y = x^3
		t = t * t * t;
		break;
	}
	case TT::CUBIC_EASE_OUT:
	{
		// y = 1 - (1 - x)^3
		t = 1 - (1 - t) * (1 - t) * (1 - t);
		break;
	}
	case TT::CUBIC_EASE_BOTH:
	{
		// y = x^3 / x^3 + (1 - x)^3
		t = (t * t * t) / ((t * t * t) + ((1 - t) * (1 - t) * (1 - t)));
		break;
	}
	default:
	{
		break;
	}
	}
}