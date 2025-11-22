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
#ifdef IMGUI_ENABLED
        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
        gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

        int count{};
        //    for (const auto& [hash, texture] : ST<MagicResourceManager>::Get()->Editor_GetTextures().Editor_GetAllResources())

        for (const auto& entry : VFS::ListDirectory(Filepaths::scenesSave))
        {
            if (!filter.PassFilter(entry)) {
                continue;
            }

            ////manual string slicing rip
            std::string sliced = entry.substr(entry.length() - 6, 6);
            if (sliced != ".scene") {
                continue;
            }

            ImGui::PushID(count++);
            {
                gui::Group group;

                if (ImGui::Button("##scene", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
                {
                    ST<SceneManager>::Get()->LoadScene(VFS::JoinPath(Filepaths::scenesSave, entry));
                }

                // Name label
                std::string displayName = entry;
                gui::ThumbnailLabel(displayName, THUMBNAIL_SIZE);

            }
            ImGui::PopID();

            grid.NextItem();
        }

#endif
    }

}
