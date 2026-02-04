#include "Editor/SceneTab.h"
#include "Assets/AssetManager.h"
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
        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (const auto& filename : VFS::ListDirectory(Filepaths::scenesSave))
        {
            if (!filter.PassFilter(filename))
                continue;
            if (VFS::GetExtension(filename) != ".scene")
                continue;

            gui::GridItem entry{ grid.Item(filename) };

            if (gui::Button{ "##scene", gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } })
                ST<SceneManager>::Get()->LoadScene(VFS::JoinPath(Filepaths::scenesSave, filename));
        }
    }

}
