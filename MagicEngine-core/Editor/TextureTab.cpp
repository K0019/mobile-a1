#include "TextureTab.h"
#include "AssetBrowser.h"
#include "EditorGuiUtils.h"

const char* TextureTab::GetName() const
{
	return "Textures";
}

const char* TextureTab::GetIdentifier() const
{
	return ICON_FA_IMAGE" Textures";
}

void TextureTab::Render()
{
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& [hash, texture] : ST<ResourceManager>::Get()->Editor_GetTextures().Editor_GetAllResources())
    {
        const std::string& materialName{ ST<ResourceManager>::Get()->Editor_GetName(hash) };
        if (!editor::MatchesFilter(materialName))
            continue;

        {
            gui::SetID id{ count++ };
            gui::Group group;

            gui::Button{ "Texture", thumbnailSizeVec2 };
            gui::PayloadSource{ "TEXTURE_HASH", hash.get() };

            gui::ThumbnailLabel(materialName, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
#endif
}