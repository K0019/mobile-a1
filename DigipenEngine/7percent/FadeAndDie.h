/******************************************************************************/
/*!
\file   FadeAndDie.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 4
\date   03/24/2025

\author Kendrick Sim Hean Guan (100%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
  This is an interface file for a component that fades out render components
  until they are fully transparent, at which point it deletes the entity.

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "IGameComponentCallbacks.h"

/*****************************************************************//*!
\class FadeAndDieComponent
\brief
	A component that fades out render components attached to the entity
	and its children until they are fully transparent, at which point
	it deletes the entity.
*//******************************************************************/
class FadeAndDieComponent
	: public IRegisteredComponent<FadeAndDieComponent>
#ifdef IMGUI_ENABLED
	, public IEditorComponent<FadeAndDieComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	FadeAndDieComponent();

	/*****************************************************************//*!
	\brief
		Updates the transparency of render components, and deletes this
		entity when fully transparent.
	\param dt
		The delta time.
	*//******************************************************************/
	void Update(float dt);

private:
	/*****************************************************************//*!
	\brief
		Draws this component to the inspector.
	\param comp
		This component.
	*//******************************************************************/
	static void EditorDraw(FadeAndDieComponent& comp);

private:
	//! Base delay before starting to fade
	float delayTimer;
	//! Random offset to the delay for each render component
	float delayRandomOffset;
	//! The length of the fade
	float fadeLength;

	//! The time remaining until full transparency per render component.
	std::vector<float> remainingLengths;

public:
	property_vtable()
};
property_begin(FadeAndDieComponent)
{
	property_var(delayTimer),
	property_var(delayRandomOffset),
	property_var(fadeLength)
}
property_vend_h(FadeAndDieComponent)

/*****************************************************************//*!
\class FadeAndDieSystem
\brief
	Updates FadeAndDieComponent.
*//******************************************************************/
class FadeAndDieSystem : public ecs::System<FadeAndDieSystem, FadeAndDieComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Constructor.
	*//******************************************************************/
	FadeAndDieSystem();

private:
	/*****************************************************************//*!
	\brief
		Updates FadeAndDieComponent.
	\param comp
		The component.
	*//******************************************************************/
	void UpdateComp(FadeAndDieComponent& comp);
};
