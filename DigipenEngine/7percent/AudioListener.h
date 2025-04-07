/******************************************************************************/
/*!
\file   AudioListener.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   04/02/2025

\author Chan Kuan Fu Ryan (100%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
  AudioListener is an ECS component-system pair that identifies an entity within
  a scene as the primary audio listener for spatial audio effects.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "IGameComponentCallbacks.h"

/*****************************************************************//*!
\class AudioListenerComponent
\brief
	Identifies an entity as the audio listener for spatial audio effects.
*//******************************************************************/
class AudioListenerComponent
	: public IRegisteredComponent<AudioListenerComponent>
	, public IGameComponentCallbacks<AudioListenerComponent>
#ifdef IMGUI_ENABLED
	, IEditorComponent<AudioListenerComponent>
#endif
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	AudioListenerComponent();

	/*****************************************************************//*!
	\brief
		Calls once when the scene is loaded to set spatial audio properties.
	*//******************************************************************/
	void OnStart() override;

	float minDistance;
	float maxDistance;
	float dopperScale;
	float distanceFactor;
	float rolloffScale;

#ifdef IMGUI_ENABLED
	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	\param comp
		The component instance.
	*//******************************************************************/
	static void EditorDraw(AudioListenerComponent& comp);
#endif
	property_vtable()
};
property_begin(AudioListenerComponent)
{
	property_var(minDistance),
	property_var(maxDistance),
	property_var(dopperScale),
	property_var(distanceFactor),
	property_var(rolloffScale),
}
property_vend_h(AudioListenerComponent)

/*****************************************************************//*!
\class AudioListenerSystem
\brief
	Does fixed update on AudioListenerComponents.
*//******************************************************************/
class AudioListenerSystem : public ecs::System<AudioListenerSystem, AudioListenerComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	AudioListenerSystem();

private:
	/*****************************************************************//*!
	\brief
		Updates AudioManager using the AudioListener's world position.
	\param comp
		The AudioListenerComponent to update.
	*//******************************************************************/
	void UpdateAudioListenerComp(AudioListenerComponent& comp);
};