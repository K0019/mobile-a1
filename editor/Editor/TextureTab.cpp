#include "Editor/TextureTab.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"
#include "renderer/gfx_material_system.h"

namespace editor {

	const AssetTabConfig TextureTab::config = {
		.name = "Textures",
		.identifier = ICON_FA_IMAGE " Textures",
		.icon = ICON_FA_IMAGE,
		.payloadType = "TEXTURE_HASH",
		.iconColor = {0.4f, 0.7f, 1.0f, 1.0f},  // Blue
		.thumbnailType = ThumbnailCache::AssetType::Texture,
		.hasThumbnails = true
	};

	const AssetTabConfig& TextureTab::GetConfig() const
	{
		return config;
	}

	void TextureTab::RenderDetailPanelContent(size_t hash, const std::string& name)
	{
#ifdef IMGUI_ENABLED
		// Texture preview
		uint64_t thumbId = ThumbnailCache::Get().GetThumbnail(hash, ThumbnailCache::AssetType::Texture, name);
		if (thumbId != 0)
		{
			float previewSize = std::min(ImGui::GetContentRegionAvail().x - 10, 120.0f);
			ImGui::Image(static_cast<ImTextureID>(thumbId), ImVec2(previewSize, previewSize), ImVec2(0, 1), ImVec2(1, 0));
		}
		else
		{
			ImGui::Button(ICON_FA_IMAGE " No Preview", ImVec2(100, 100));
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Get texture info from ResourceTexture handle
		auto& container = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceTexture>();
		ResourceTexture* texResource = const_cast<ResourceTexture*>(static_cast<const ResourceTexture*>(container.GetResource(hash)));

		if (texResource && texResource->IsLoaded())
		{
			// Access texture hot data via ResourceManager
			auto& resourceManager = ST<GraphicsMain>::Get()->GetAssetSystem();
			const auto* hotData = resourceManager.getTextureHotData(texResource->handle);

			if (hotData && hotData->hasGfxTexture.load(std::memory_order_acquire))
			{
				// Get metadata from renderer's material system using gfx handle
				auto* renderer = ST<GraphicsMain>::Get()->GetRenderer();
				if (renderer)
				{
					// Construct gfx handle from hot data indices
					gfx::TextureHandle gfxHandle;
					gfxHandle.index = static_cast<uint16_t>(hotData->gfxTextureIndex);
					gfxHandle.generation = static_cast<uint16_t>(hotData->gfxTextureGeneration);

					const gfx::TextureEntry* texEntry = renderer->getMaterialSystem().getTexture(gfxHandle);
					if (texEntry && texEntry->inUse)
					{
						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Resolution:");
						ImGui::SameLine(100);
						ImGui::Text("%u x %u", texEntry->width, texEntry->height);

						ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Color:");
						ImGui::SameLine(100);
						ImGui::Text("%s", texEntry->isSRGB ? "sRGB" : "Linear");
					}
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), ICON_FA_HOURGLASS " Loading...");
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Not loaded");
			ImGui::TextDisabled("Texture is registered but not in GPU memory.");
		}
#endif
	}

}
