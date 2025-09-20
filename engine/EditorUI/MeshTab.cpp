#include "MeshTab.h"
#include "AssetBrowser.h"
#include "EditorGuiUtils.h"

const char* MeshTab::GetName() const
{
	return "Meshes";
}

const char* MeshTab::GetIdentifier() const
{
	return ICON_FA_IMAGE" Meshes";
}

void MeshTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& [hash, mesh] : ST<ResourceManager>::Get()->Editor_GetMeshes().Editor_GetAllResources())
    {
        if (!editor::MatchesFilter(mesh.get().name))
            continue;

        {
            gui::SetID id{ count++ };
            gui::Group group;

            gui::Button{ "Mesh", thumbnailSizeVec2 };
            gui::PayloadSource{ "RESOURCE_HASH", mesh.get().hash, mesh.get().name.c_str() };

            gui::ThumbnailLabel(mesh.get().name, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
}