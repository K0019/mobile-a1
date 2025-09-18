/******************************************************************************/
/*!
\file   KillWhenAnimationFinish.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   03/11/2025

\author Marc Alviz Evangelista (100%)
\par    email: marcalviz.e\@digipen.edu
\par    DigiPen login: marcalviz.e

\brief

	Deletes the animation entity when the animation finishes


All content ? 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
class KillWhenAnimationFinishComponent : public IRegisteredComponent<KillWhenAnimationFinishComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	\return
	*//******************************************************************/
	KillWhenAnimationFinishComponent();

	/*****************************************************************//*!
	\brief
		Default destructor.
	\return
	*//******************************************************************/
	~KillWhenAnimationFinishComponent();

	property_vtable()
};
property_begin(KillWhenAnimationFinishComponent)
{

}
property_vend_h(KillWhenAnimationFinishComponent)

class AnimationFinishSystem : public ecs::System<AnimationFinishSystem, KillWhenAnimationFinishComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	\return
	*//******************************************************************/
	AnimationFinishSystem();

private:

	/*****************************************************************//*!
	\brief
		Updates the system
	\param[in, out] comp
		Component to update
	\return
	*//******************************************************************/
	void Update(KillWhenAnimationFinishComponent& comp);
};
