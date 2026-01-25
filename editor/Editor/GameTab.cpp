#include "Editor/GameTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/WeaponCreationWindow.h"

namespace editor {

    const char* GameTab::GetName() const
    {
        return "Game Resouces";
    }

    const char* GameTab::GetIdentifier() const
    {
        return ICON_FA_GAMEPAD" Game Resources";
    }

    void GameTab::Render(const gui::TextBoxWithFilter& filter)
    {
        if (gui::Button{ "Create New Weapon" })
            editor::CreateGuiWindow<editor::WeaponCreationWindow>("", nullptr);
        gui::Separator();

        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = gui::GetAvailableContentRegion().x; // random offset
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
        gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, weapon] : ST<MagicResourceManager>::Get()->Editor_GetContainer<WeaponInfo>().Editor_GetAllResources())
        {
            const std::string& weaponName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(weaponName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                gui::Button{ "Weapon", thumbnailSizeVec2 };
                gui::PayloadSource{ "GAME_WEAPON_HASH", hash.get() };
                if (gui::ItemContextMenu contextMenu{ "WeaponMenu" })
                {
                    if (gui::MenuItem("Edit"))
                        editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), UserResourceHandle<WeaponInfo>{ hash }.GetResource());
                }

                gui::ShowSimpleHoverTooltip(weaponName);
                gui::ThumbnailLabel(weaponName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }
    }

}
