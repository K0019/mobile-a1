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

        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        int count{};
        for (const auto& [hash, weapon] : ST<MagicResourceManager>::Get()->Editor_GetContainer<WeaponInfo>().Editor_GetAllResources())
        {
            const std::string& weaponName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(weaponName))
                continue;

            gui::GridItem entry{ grid.Item() };

            gui::Button{ "Weapon", gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } };
            gui::PayloadSource{ "GAME_WEAPON_HASH", hash.get() };
            if (gui::ItemContextMenu contextMenu{ "WeaponMenu" })
            {
                if (gui::MenuItem("Edit"))
                    editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), UserResourceHandle<WeaponInfo>{ hash }.GetResource());
            }

            gui::ShowSimpleHoverTooltip(weaponName);
            gui::ThumbnailLabel(weaponName, AssetBrowser::THUMBNAIL_SIZE);
        }
    }

}
