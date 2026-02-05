#include "Editor/GameTab.h"
#include "Editor/AssetBrowserSettings.h"
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
#ifdef IMGUI_ENABLED
        if (gui::Button{ "Create New Weapon" })
            editor::CreateGuiWindow<editor::WeaponCreationWindow>("", nullptr);
        gui::Separator();

        const bool listMode = (AssetBrowserSettings::viewMode == AssetBrowserSettings::ViewMode::List);
        const float thumbnailSize = AssetBrowserSettings::thumbnailSize;
        const float panelWidth = gui::GetAvailableContentRegion().x;
        gui::GridHelper grid{ panelWidth, thumbnailSize + 10 };

        gui::SetStyleVar itemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 5, 5 } };
        gui::SetStyleVar framePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 2, 2 } };

        const ImVec4 weaponColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);  // Red

        int count{};
        for (const auto& [hash, weapon] : ST<AssetManager>::Get()->Editor_GetContainer<WeaponInfo>().Editor_GetAllResources())
        {
            const std::string& weaponName{ *ST<AssetManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(weaponName))
                continue;

            gui::SetID id{ count++ };

            if (listMode)
            {
                if (ImGui::Selectable(("##sel" + weaponName).c_str(), false, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(0, 20)))
                {
                    if (auto* weaponPtr = AssetHandle<WeaponInfo>{ hash }.GetResource())
                        editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), weaponPtr);
                }

                gui::PayloadSource{ "GAME_WEAPON_HASH", hash.get() };

                if (gui::ItemContextMenu contextMenu{ "WeaponMenu" })
                {
                    if (gui::MenuItem("Edit"))
                    {
                        if (auto* weaponPtr = AssetHandle<WeaponInfo>{ hash }.GetResource())
                            editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), weaponPtr);
                    }
                }

                ImGui::SameLine(0, 0);
                ImGui::SetCursorPosX(8);
                ImGui::TextColored(weaponColor, ICON_FA_GUN);
                ImGui::SameLine();
                ImGui::Text("%s", weaponName.c_str());
            }
            else
            {
                gui::Group group;

                ImGui::PushStyleColor(ImGuiCol_Text, weaponColor);
                gui::Button{ ICON_FA_GUN "##weapon", gui::Vec2{ thumbnailSize, thumbnailSize } };
                ImGui::PopStyleColor();

                gui::PayloadSource{ "GAME_WEAPON_HASH", hash.get() };

                if (gui::ItemContextMenu contextMenu{ "WeaponMenu" })
                {
                    if (gui::MenuItem("Edit"))
                    {
                        if (auto* weaponPtr = AssetHandle<WeaponInfo>{ hash }.GetResource())
                            editor::CreateGuiWindow<editor::WeaponCreationWindow>(weaponName.substr(0, weaponName.size() - 1), weaponPtr);
                    }
                }

                gui::ThumbnailLabel(weaponName, thumbnailSize);
            }
            grid.NextItem();
        }
#endif
    }

    void GameTab::RenderSidebar()
    {
        gui::Child sidebar{ "GameSidebar", gui::Vec2{ 100.0f, 0.0f }, gui::FLAG_CHILD::BORDERS };

        for (SUBTAB_TYPE i{}; i < SUBTAB_TYPE::TOTAL; ++i)
            if (gui::Selectable(SUBTAB_NAMES[+i], currentSubtab == i))
                currentSubtab = i;
    }

}
