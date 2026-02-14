#include "Editor/GameTab.h"
#include "Editor/AssetBrowserSettings.h"
#include "Editor/EditorGuiUtils.h"

#define X(type, name) name,
const std::array<const char*, +editor::GameTab::SUBTAB_TYPE::TOTAL> editor::GameTab::SUBTAB_NAMES{
    GAMETAB_SUBTAB_TYPE_ENUM
};

namespace editor {

    GameTab::GameTab()
        : currentSubtab{ SUBTAB_TYPE::TOTAL }
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
