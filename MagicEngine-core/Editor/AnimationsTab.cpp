#include "Editor/AnimationsTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

namespace editor
{
    const char* AnimationsTab::GetName() const
    {
        return "Animations";
    }

    const char* AnimationsTab::GetIdentifier() const
    {
        return ICON_FA_IMAGE" Animations";
    }

    void AnimationsTab::Render(const gui::TextBoxWithFilter& filter)
    {
#ifdef IMGUI_ENABLED
        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
        gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, anim] : ST<MagicResourceManager>::Get()->Editor_GetContainer<ResourceAnimation>().Editor_GetAllResources())
        {
            const std::string& animName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(animName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                gui::Button{ "Anim", thumbnailSizeVec2 };
                gui::PayloadSource{ "ANIMATION_HASH", hash.get() };

                gui::ShowSimpleHoverTooltip(animName);
                gui::ThumbnailLabel(animName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }
#endif
    }
}