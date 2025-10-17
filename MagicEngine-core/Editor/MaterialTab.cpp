#include "Editor/MaterialTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"

#include "Editor/MaterialCreation.h"

const char* MaterialTab::GetName() const
{
    return "Materials";
}

const char* MaterialTab::GetIdentifier() const
{
    return ICON_FA_IMAGE" Materials";
}

void MaterialTab::Render()
{
#ifdef IMGUI_ENABLED
    if (gui::Button{ "Create Material" })
        editor::CreateGuiWindow<editor::MaterialCreationWindow>();
    gui::Separator();

    
    float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
    gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& [hash, material] : ST<MagicResourceManager>::Get()->Editor_GetMaterials().Editor_GetAllResources())
    {
        const std::string& materialName{ ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
        if (!editor::MatchesFilter(materialName))
            continue;

        {
            gui::SetID id{ count++ };
            gui::Group group;

            gui::Button{ "Material", thumbnailSizeVec2 };
            gui::PayloadSource{ "MATERIAL_HASH", hash.get() };

            gui::ThumbnailLabel(materialName, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
#endif
}
