#include "Editor/SceneTab.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/SceneManagement.h"
#include "Editor/AssetBrowser.h"
#include "FilepathConstants.h"

#include "VFS/VFS.h"

namespace editor {

    ///////////////
    ///  Scene  ///
    ///////////////
    const char* SceneTab::GetName() const
    {
        return "Scenes";
    }

    const char* SceneTab::GetIdentifier() const
    {
        return ICON_FA_DIAMOND" Scenes";
    }

    void SceneTab::Render(const gui::TextBoxWithFilter& filter)
    {
        const float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        const float panelWidth = gui::GetAvailableContentRegion().x;
        gui::GridHelper grid{ panelWidth, THUMBNAIL_SIZE + 10 };

        gui::SetStyleVar itemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 5, 5 } };
        gui::SetStyleVar framePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 2, 2 } };

        int count{};
        for (const auto& entry : VFS::ListDirectory(Filepaths::scenesSave))
        {
            if (!filter.PassFilter(entry))
                continue;
            if (VFS::GetExtension(entry) != ".scene")
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                if (gui::Button{ "##scene", gui::Vec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE } })
                    ST<SceneManager>::Get()->LoadScene(VFS::JoinPath(Filepaths::scenesSave, entry));

                // Name label
                gui::ThumbnailLabel(entry, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }
    }

}
