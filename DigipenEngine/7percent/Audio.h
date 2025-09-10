/******************************************************************************/
/*!
\file   Audio.h
\par    Project: Kuro Mahou
\par    Course: CSD3401
\par    Section B
\date   10/09/2025

\author Kuan Yew Chong (100%)
\par    email: yewchong.k\@digipen.edu
\par    DigiPen login: yewchong.k

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
#include "IGameComponentCallbacks.h"

class AudioSourceComponent
	: public IRegisteredComponent<AudioSourceComponent>
#ifdef IMGUI_ENABLED
	, public IEditorComponent<AudioSourceComponent>
#endif
	, public IGameComponentCallbacks<AudioSourceComponent>
{
public:
	AudioSourceComponent();
	void OnStart() override;
	void Play(AudioType a, std::string name);

	float minDistance;
	float maxDistance;
	float dopperScale;
	float distanceFactor;
	float rolloffScale;

private:
	FMOD::Channel* channel = nullptr;
	virtual void EditorDraw() override;

	property_vtable()
};
property_begin(AudioSourceComponent)
{
	property_var(minDistance),
	property_var(maxDistance),
	property_var(dopperScale),
	property_var(distanceFactor),
	property_var(rolloffScale),
}
property_vend_h(AudioSourceComponent)

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
	void UpdateComp(AudioSourceComponent& comp);
};