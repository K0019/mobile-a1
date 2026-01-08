#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Engine/Resources/Types/ResourceTypesAudio.h"

namespace editor {

	struct SoundTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;

	private:
		void RenderSoundContextMenu(size_t hash, const std::string& name);

	private:
		uint32_t currentPreviewSound = 0; /**< Current preview sound channel */
		size_t lastPreviewAudioHash; /**< Current preview audio information */
		bool use3DMode = false;
		bool queueFade = false;
	};

}
