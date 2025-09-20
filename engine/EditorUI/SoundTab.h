#pragma once
#include "AssetBrowserCategories.h"
#include "Audio.h"

struct SoundTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	void RenderSoundContextMenu(std::string const& name);

private:
	uint32_t currentPreviewSound = 0; /**< Current preview sound channel */
	AudioAsset lastPreviewAudio; /**< Current preview audio information */
	bool use3DMode = false;
};
