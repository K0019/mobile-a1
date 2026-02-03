#include "Editor/GameTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/WeaponCreationWindow.h"

#define X(type, name) name,
const std::array<const char*, +editor::GameTab::SUBTAB_TYPE::TOTAL> editor::GameTab::SUBTAB_NAMES{
    GAMETAB_SUBTAB_TYPE_ENUM
};

namespace editor {

    GameTab::GameTab()
        : currentSubtab{ SUBTAB_TYPE::WEAPONS }
    {
    }

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
        RenderSidebar();

        gui::SameLine();
        gui::Child child{ "MainTabWindow" };
        switch (currentSubtab)
        {
        case SUBTAB_TYPE::WEAPONS: RenderWeapons(filter); break;
        }
    }

    void GameTab::RenderWeapons(const gui::TextBoxWithFilter& filter)
    {
        if (gui::Button{ "Create New Weapon" })
            editor::CreateGuiWindow<editor::WeaponCreationWindow>("", nullptr);
        gui::Separator();

        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (const auto& [hash, weapon] : ST<MagicResourceManager>::Get()->Editor_GetContainer<WeaponInfo>().Editor_GetAllResources())
        {
            const std::string& weaponName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(weaponName))
                continue;

            gui::GridItem entry{ grid.Item(weaponName) };

            gui::Button{ "Weapon", gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } };
            gui::PayloadSource{ "GAME_WEAPON_HASH", hash.get() };
            if (gui::ItemContextMenu contextMenu{ "WeaponMenu" })
            {
                if (gui::MenuItem("Edit"))
                    editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), UserResourceHandle<WeaponInfo>{ hash }.GetResource());
            }
        }
    }

    void GameTab::RenderSidebar()
    {
        gui::Child sidebar{ "GameSidebar", gui::Vec2{ 100.0f, 0.0f }, gui::FLAG_CHILD::BORDERS };

        for (SUBTAB_TYPE i{}; i < SUBTAB_TYPE::TOTAL; ++i)
            if (gui::Selectable(SUBTAB_NAMES[+i], currentSubtab == i))
                currentSubtab = i;
    }

}
