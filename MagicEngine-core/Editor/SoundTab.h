#pragma once
#include "AssetBrowserCategories.h"
#include "ResourceTypesAudio.h"

struct SoundTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	void RenderSoundContextMenu(size_t hash, const std::string& name);

private:
	uint32_t currentPreviewSound = 0; /**< Current preview sound channel */
	size_t lastPreviewAudioHash; /**< Current preview audio information */
	bool use3DMode = false;
};
