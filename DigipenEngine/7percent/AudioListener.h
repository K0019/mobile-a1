#pragma once
#include "IGameComponentCallbacks.h"

// Listener is more of a tag component than anything. It just indicates this entity is the listener
class AudioListenerComponent
	: public IRegisteredComponent<AudioListenerComponent>
	, public IEditorComponent<AudioListenerComponent>
	, public IGameComponentCallbacks<AudioListenerComponent>
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

	/*****************************************************************//*!
	\brief
		Editor draw function, draws the IMGui elements to allow the
		component's values to be edited. Disabled when IMGui is disabled.
	*//******************************************************************/
	virtual void EditorDraw() override;

	property_vtable()
};
property_begin(AudioListenerComponent)
{
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