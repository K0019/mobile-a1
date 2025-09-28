#include "SceneTab.h"
#include "ResourceManager.h"
#include "EditorGuiUtils.h"
#include "SceneManagement.h"


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

void SceneTab::Render()
{
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->scenesSave })
    {
        if (!editor::MatchesFilter(entry.path().string()))
            continue;
        if (entry.path().extension() != ".scene")
            continue;

        ImGui::PushID(count++);
        {
            gui::Group group;

            if (ImGui::Button("##scene", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
            {
                ST<SceneManager>::Get()->LoadScene(entry.path());
            }

            // Name label
            std::string displayName = entry.path().stem().string();
            gui::ThumbnailLabel(displayName, THUMBNAIL_SIZE);

        }
        ImGui::PopID();

        grid.NextItem();
    }
#endif
}
