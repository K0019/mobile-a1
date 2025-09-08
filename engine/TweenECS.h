/******************************************************************************/
/*!
\file   TweenECS.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  Declares all classes required to integrate tweens into ECS.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "TweenBase.h"

// TweenECS - A modified Tween
/*****************************************************************//*!
\class TweenECS
\brief
	Template class inheriting from TweenBase. Stores a handle to the
	entity that it tweens, amongst other things.
*//******************************************************************/
template <typename V, typename U, typename Object>
class TweenECS : public TweenBase
{
public:
	/*****************************************************************//*!
	\brief
		Constructor to store values.
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
	TweenECS(
		ecs::EntityHandle entity,			// Pointer to entity
		void (Object::* setter)(V),			// Function pointer to object's setter
		U const& vStart,					// Start value
		U const& vEnd,						// End value
		float duration,						// Duration of tween
		TT type);							// Type of tween

	/*****************************************************************//*!
	\brief
		Tells you if this tween is currently active.
	\return
		True if active. False otherwise.
	*//******************************************************************/
	bool IsActive() const override;

	/*****************************************************************//*!
	\brief
		Unlike regular Tween update, ECS tweens will try to first acquire
		the component related to the setter function. If the setter comes
		from Transform, it just grabs the entity's transform.
	\param dt
		Time elapsed since the previous frame.
	*//******************************************************************/
	void Update(float dt) override;

	/*****************************************************************//*!
	\brief
		Gonna be honest, I forgot why I even coded this. But you can get
		the setter function if you want. The syntax is super weird.
	\return
		The stored setter function pointer.
	*//******************************************************************/
	void (Object::* GetSetter())(V);

private:
	ecs::EntityHandle entity;
	void (Object::* setter)(V);
	U vStart;
	U vDelta;
	float duration;
	float elapsed;
	TT type;
	bool active;
};

// TweenComponent - Basically a TweenManager for ECS
/*****************************************************************//*!
\class TweenComponent
\brief
	Tweening Component to attach to entities.
*//******************************************************************/
class TweenComponent
{
public:
	/*****************************************************************//*!
	\brief
		Default initialises tweens.
	*//******************************************************************/
	TweenComponent();

	/*****************************************************************//*!
	\brief
		Copy constructor for debug.
	\param other
		The component to copy.
	*//******************************************************************/
	TweenComponent(TweenComponent const& other) noexcept;

	/*****************************************************************//*!
	\brief
		Move constructor for debug.
	\param other
		The component to move.
	*//******************************************************************/
	TweenComponent(TweenComponent&& other) noexcept;

	/*****************************************************************//*!
	\brief
		Calls Clean().
	*//******************************************************************/
	~TweenComponent();

	/*****************************************************************//*!
	\brief
		Starts tweens for the attached entity. The new tween will replace
		previous tweens using the same setter (interpolating the same
		value) to prevent multiple tweens cancelling each other out. Then
		it is added to the vector of tweens.
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
	template<typename V, typename U, typename Object>
	void StartTween(
		void (Object::* setter)(V),			// Function pointer to object's setter
		U const& vStart,					// Start value
		U const& vEnd,						// End value
		float duration,						// Duration of tween
		TT type);							// Type of tween

	/*****************************************************************//*!
	\brief
		Updates all tweens in the vector and removes inactive tweens.
	\param dt
		Time elapsed since the previous frame.
	*//******************************************************************/
	void Update(float dt);

	/*****************************************************************//*!
	\brief
		Deletes all tweens and clears the vector.
	*//******************************************************************/
	void Clean();

	/*****************************************************************//*!
	\brief
		Gets the number of active tweens in this component.
	\return
		Unsigned integral value.
	*//******************************************************************/
	size_t GetNumberOfTweens();

private:
	std::vector<TweenBase*> tweens;
};

// TweenSystem - System for ECS
/*****************************************************************//*!
\class TweenSystem
\brief
	Handles updating of TweenComponents inside of ECS.
*//******************************************************************/
class TweenSystem : public ecs::System<TweenSystem, TweenComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Initialises TweenSystem to work with ECS.
	*//******************************************************************/
	TweenSystem();

private:
	/*****************************************************************//*!
	\brief
		Updates the TweenComponent.
	\param tweenComp
		TweenComponent reference to update.
	*//******************************************************************/
	void Update(TweenComponent& tweenComp);
};

#include "TweenECS.ipp"
