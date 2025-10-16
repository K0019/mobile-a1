#pragma once
#include "IRegisteredComponent.h"

// Listener is more of a tag component than anything. It just indicates this entity is the listener
class AudioListenerComponent
	: public IRegisteredComponent<AudioListenerComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	AudioListenerComponent();

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