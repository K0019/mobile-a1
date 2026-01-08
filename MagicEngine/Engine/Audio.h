/******************************************************************************/
/*!
\file   Audio.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Section B
\date   10/09/2025

\author Kuan Yew Chong (90%)
\par    email: yewchong.k\@digipen.edu
\par    DigiPen login: yewchong.k

\author Elijah Teo (10%)
\par    email: teo.e\@digipen.edu
\par    DigiPen login: teo.e

\brief
	The audio system is responsible for ensuring FMOD gets a update call every frame.
	Otherwise, the components do not have to be updated every frame. Instead it is most likely to be invoked by scripting calling this component's play function,
	leaving the component more like a data container for audio playback.

	It also controls the 3D listener attributes, as they are unique to each channel

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Engine/Resources/Types/ResourceTypesAudio.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"

class AudioSourceComponent
	: public IRegisteredComponent<AudioSourceComponent>
	, public IEditorComponent<AudioSourceComponent>
{
public:
	AudioSourceComponent();

	void Play(AudioType a);

//FOR LUAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
public:
	// (Optional) convenience overload for Lua:

	// ---- Getters / setters for Lua ----
	float GetMinDistance()   const { return minDistance; }
	void  SetMinDistance(float v) { minDistance = v; }

	float GetMaxDistance()   const { return maxDistance; }
	void  SetMaxDistance(float v) { maxDistance = v; }

	float GetDopplerScale()  const { return dopperScale; }
	void  SetDopplerScale(float v) { dopperScale = v; }

	float GetDistanceFactor() const { return distanceFactor; }
	void  SetDistanceFactor(float v) { distanceFactor = v; }

	float GetRolloffScale()  const { return rolloffScale; }
	void  SetRolloffScale(float v) { rolloffScale = v; }

	size_t GetAudioFile()    const { return audioFile; }
	void   SetAudioFile(size_t id) { audioFile = id; }

	bool   IsPlaying()       const { return isPlaying; }
	void   SetIsPlaying(bool b) { isPlaying = b; } // if you want Lua to control flag

	float  GetVolumeModifier() const { return volumeModifier; }
	void   SetVolumeModifier(float v) { volumeModifier = v; }

	AudioType GetAudioCategory() const { return audioCategory; }
	void   SetAudioCategory(AudioType category) { audioCategory = category; }

private:
	virtual void EditorDraw() override;

private:
	float minDistance;
	float maxDistance;
	float dopperScale;
	float distanceFactor;
	float rolloffScale;
	float volumeModifier;
	AudioType audioCategory;
	size_t audioFile;
	bool isPlaying;
	uint32_t channelHandle;

	property_vtable()
};
property_begin(AudioSourceComponent)
{
	property_var(minDistance),
		property_var(maxDistance),
		property_var(dopperScale),
		property_var(distanceFactor),
		property_var(rolloffScale),
		property_var(volumeModifier),
		property_var_fnbegin("audioCategory", char)
			if (isRead) InOut = static_cast<char>(Self.audioCategory);
			else Self.audioCategory = static_cast<AudioType>(InOut);
		property_var_fnend(),
		property_var(audioFile),

}
property_vend_h(AudioSourceComponent)

// Listener is more of a tag component than anything. It just indicates this entity is the listener
class AudioListenerComponent
	: public IRegisteredComponent<AudioListenerComponent>
	, public IEditorComponent<AudioListenerComponent>
{
public:
	/*****************************************************************//*!
	\brief
		Default constructor.
	*//******************************************************************/
	AudioListenerComponent();

private:
	virtual void EditorDraw() override;

	property_vtable()
};
property_begin(AudioListenerComponent)
{
}
property_vend_h(AudioListenerComponent)

/*****************************************************************//*!
\class AudioSystem
\brief
	ECS System.
*//******************************************************************/
class AudioSystem : public ecs::System<AudioSystem>
{
public:
	/*****************************************************************//*!
	\brief
		Update function that is called continuously.
	*//******************************************************************/
	bool PreRun() override;

};