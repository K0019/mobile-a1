#include "Engine/Audio.h"
#include "Managers/AudioManager.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/Containers/GUICollection.h"

AudioSourceComponent::AudioSourceComponent()
	: minDistance{ 20.0f }
	, maxDistance{ 50.0f }
	, dopperScale{ 1.0f }
	, distanceFactor{ 0.01f }
	, rolloffScale{ 0.1f }
	, volumeModifier{ 1.0f }
	, audioCategory{ AudioType::SFX }
	, audioFile{ 0 }
	, isPlaying{ false }
	, channelHandle{}
{
}

void AudioSourceComponent::Play(AudioType a)
{
	// Use the provided parameter if not END, otherwise use the component's audioCategory
	AudioType categoryToUse = (a != AudioType::END) ? a : audioCategory;
	channelHandle = ST<AudioManager>::Get()->PlaySound(audioFile, false, categoryToUse, volumeModifier);
}

void AudioSourceComponent::EditorDraw()
{
	// Audio Category Dropdown
	static const char* audioCategoryNames[] = { "BGM", "SFX", "Default" };
	int categoryIndex = static_cast<int>(audioCategory);
	gui::Combo{ "Audio Category", audioCategoryNames, 3, &categoryIndex };
	audioCategory = static_cast<AudioType>(categoryIndex);

	gui::VarDrag("Minimum Distance", &minDistance);
	gui::VarDrag("Maximum Distance", &maxDistance);
	gui::VarDrag("Doppler Scale", &dopperScale);
	gui::VarDrag("Distance Factor", &distanceFactor);
	gui::VarDrag("Rolloff Scale", &rolloffScale);
	gui::VarDrag("Volume Modifier", &volumeModifier);

	if (audioFile != 0) {

		gui::TextBoxReadOnly("Audio File", *ST<MagicResourceManager>::Get()->Editor_GetName(audioFile));
		gui::PayloadTarget<size_t>("SOUND_HASH", [this](size_t hash) -> void {
			audioFile = hash;
			});
	}
	else {
		gui::TextBoxReadOnly("Audio File", "");
		gui::PayloadTarget<size_t>("SOUND_HASH", [this](size_t hash) -> void {
			audioFile = hash;
			});
	}
	if (!isPlaying) {
		if (gui::Button("Play Audio", gui::Vec2{ 100.0f, 30.0f })) {
			if (!ST<AudioManager>::Get()->IsPlaying(channelHandle))
			{
				channelHandle = ST<AudioManager>::Get()->PlaySound(audioFile, false, audioCategory, volumeModifier);
				isPlaying = true;
			}
		}
	}
	else {
		if (gui::Button("Stop Audio", gui::Vec2{ 100.0f, 30.0f })) {
			if (ST<AudioManager>::Get()->IsPlaying(channelHandle))
			{
				ST<AudioManager>::Get()->StopSound(channelHandle);
				isPlaying = false;
			}
		}
	}



	//gui::VarDefaultTextBox("Audio Resource", &audioFile);
}

AudioListenerComponent::AudioListenerComponent()
{
}

void AudioListenerComponent::EditorDraw()
{
	gui::TextUnformatted("Make sure you only have one of this component in the scene!");
}

bool AudioSystem::PreRun()
{
	ST<AudioManager>::Get()->Update();
	return true;
}
