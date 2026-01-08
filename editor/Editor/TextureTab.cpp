#include "Editor/TextureTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

namespace editor {

    const char* TextureTab::GetName() const
    {
        return "Textures";
    }

    const char* TextureTab::GetIdentifier() const
    {
        return ICON_FA_IMAGE" Textures";
    }

    void TextureTab::Render(const gui::TextBoxWithFilter& filter)
    {
#ifdef IMGUI_ENABLED
        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
        gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, texture] : ST<MagicResourceManager>::Get()->Editor_GetContainer<ResourceTexture>().Editor_GetAllResources())
        {
            const std::string& textureName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(textureName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                gui::Button{ "Texture", thumbnailSizeVec2 };
                gui::PayloadSource{ "TEXTURE_HASH", hash.get() };

                gui::ShowSimpleHoverTooltip(textureName);
                gui::ThumbnailLabel(textureName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }
#endif
    }

}
