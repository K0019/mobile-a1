#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypesAudio.h"

namespace editor {

	class SoundTab : public GenericAssetTab<ResourceAudio>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderToolbar() override;
		void RenderContextMenuItems(size_t hash) override;
		void RenderDetailPanelContent(size_t hash, const std::string& name) override;

	private:
		void PlayPreview(size_t hash);
		void StopPreview();

	private:
		static const AssetTabConfig config;

		uint32_t currentPreviewSound = 0;
		size_t lastPreviewAudioHash = 0;
		bool use3DMode = false;
		bool queueFade = false;

		float previewVolume = 1.f;
		Vec3 pos = Vec3{ 0.f, 0.f, 0.f };
		Vec3 vel = Vec3{ 0.f, 0.f, 0.f };
		float animationTimer = 0.0f;
		int tildeCount = 0;
	};

}
