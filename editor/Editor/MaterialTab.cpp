#include "Editor/MaterialTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/ThumbnailCache.h"
#include "Assets/Types/AssetTypes.h"

#include "Editor/MaterialCreation.h"

namespace editor {

    const char* MaterialTab::GetName() const
    {
        return "Materials";
    }

    const char* MaterialTab::GetIdentifier() const
    {
        return ICON_FA_PALETTE" Materials";
    }

    void MaterialTab::Render(const gui::TextBoxWithFilter& filter)
    {
        if (gui::Button{ "Create Material" })
            editor::CreateGuiWindow<editor::MaterialCreationWindow>();
        gui::SameLine();
        gui::TextDisabled("(?)");
        gui::ShowSimpleHoverTooltip("Click a material to see which meshes use it");

        gui::Separator();

        float availableHeight = gui::GetAvailableContentRegion().y;
        float detailPanelHeight = selectedMaterialHash.has_value() ? 150.0f : 0.0f;
        float gridHeight = availableHeight - detailPanelHeight;

        // Grid view (top section)
        RenderGridView(filter, gridHeight);

        // Detail panel (bottom section)
        if (selectedMaterialHash.has_value())
        {
            gui::Separator();
            RenderDetailPanel();
        }
    }

    void MaterialTab::RenderGridView(const gui::TextBoxWithFilter& filter, float height)
    {
        gui::Child gridChild{ "MaterialGrid", gui::Vec2{ 0.0f, height } };

        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (const auto& [hash, material] : ST<AssetManager>::Get()->Editor_GetContainer<ResourceMaterial>().Editor_GetAllResources())
        {
            const std::string& materialName{ *ST<AssetManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(materialName))
                continue;

            gui::GridItem item{ grid.Item(materialName) };

            bool isSelected{ selectedMaterialHash.has_value() && selectedMaterialHash.value() == hash.get() };
            gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.6f }, isSelected };

            // Try to get thumbnail from cache (uses material's baseColor texture)
            uint64_t thumbId = ThumbnailCache::Get().GetThumbnail(
                hash.get(), ThumbnailCache::AssetType::Material, materialName);

            bool clicked = false;
            if (thumbId != 0)
            {
                // Display actual thumbnail (ImTextureID is ImU64)
                clicked = gui::ImageButton{
                    ("##mat" + std::to_string(grid.GetItemCount())).c_str(),
                    thumbId,
                    gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE }
                };
            }
            else
            {
                // Fall back to icon
                clicked = gui::Button{
                    (std::string{ ICON_FA_PALETTE"##" } + std::to_string(grid.GetItemCount())).c_str(),
                    gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE }
                };
            }

            if (clicked)
            {
                if (isSelected)
                    selectedMaterialHash.reset();
                else
                    selectedMaterialHash = hash.get();
            }

            // Context menu for edit
            if (gui::ItemContextMenu{ materialName.c_str() })
                if (gui::MenuItem(ICON_FA_FILE_IMPORT" Edit Material"))
                    editor::CreateGuiWindow<editor::MaterialEditWindow>(hash);
            gui::PayloadSource payloadSource{ "MATERIAL_HASH", hash.get() };
        }
    }

    void MaterialTab::RenderDetailPanel()
    {
#ifdef IMGUI_ENABLED
        if (!selectedMaterialHash.has_value())
            return;

        ImGui::BeginChild("MaterialDetails", ImVec2(0, 0), true);

        size_t hash = selectedMaterialHash.value();
        const std::string* matName = ST<AssetManager>::Get()->Editor_GetName(hash);

        if (!matName)
        {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Material not found");
            ImGui::EndChild();
            return;
        }

        // Header with close button
        ImGui::Text(ICON_FA_PALETTE " %s", matName->c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        if (ImGui::SmallButton(ICON_FA_XMARK))
        {
            selectedMaterialHash.reset();
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 300);

        // Left column - Meshes using this material
        ImGui::Text(ICON_FA_CUBE " Used by meshes:");
        ImGui::BeginChild("MeshUsageList", ImVec2(0, 80), true);

        int usageCount = 0;
        for (const auto& [meshHash, meshRef] : ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
        {
            // Check if this mesh uses the selected material
            const ResourceMesh& mesh = meshRef.get();
            bool usesMaterial = false;
            for (size_t matHash : mesh.defaultMaterialHashes)
            {
                if (matHash == hash)
                {
                    usesMaterial = true;
                    break;
                }
            }

            if (usesMaterial)
            {
                const std::string* meshName = ST<AssetManager>::Get()->Editor_GetName(meshHash);
                if (meshName)
                {
                    ImGui::Selectable(meshName->c_str(), false);

                    // Drag source for mesh
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        size_t dragHash = meshHash.get();
                        ImGui::SetDragDropPayload("MESH_HASH", &dragHash, sizeof(size_t));
                        ImGui::Text("Mesh: %s", meshName->c_str());
                        ImGui::EndDragDropSource();
                    }
                }
                usageCount++;
            }
        }

        if (usageCount == 0)
        {
            ImGui::TextDisabled("Not used by any mesh");
        }

        ImGui::EndChild();

        ImGui::NextColumn();

        // Right column - Actions
        ImGui::Text("Actions:");
        ImGui::Spacing();

        if (ImGui::Button(ICON_FA_PEN " Edit Material", ImVec2(-1, 0)))
        {
            editor::CreateGuiWindow<editor::MaterialEditWindow>(hash);
        }

        ImGui::Button(ICON_FA_HAND " Drag to assign", ImVec2(-1, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("MATERIAL_HASH", &hash, sizeof(size_t));
            ImGui::Text("Material: %s", matName->c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::Columns(1);

        ImGui::EndChild();
#endif
    }

}
