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
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& [hash, mesh] : ST<ResourceManager>::Get()->Editor_GetMeshes().Editor_GetAllResources())
    {
        const std::string& meshName{ ST<ResourceManager>::Get()->Editor_GetName(hash) };
        if (!editor::MatchesFilter(meshName))
            continue;

        {
            gui::SetID id{ count++ };
            gui::Group group;

            gui::Button{ "Mesh", thumbnailSizeVec2 };
            gui::PayloadSource{ "MESH_HASH", hash.get() };

            gui::ThumbnailLabel(meshName, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
#endif
}