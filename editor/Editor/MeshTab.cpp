#include "Editor/MeshTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

namespace editor {

    const char* MeshTab::GetName() const
    {
        return "Meshes";
    }

    const char* MeshTab::GetIdentifier() const
    {
        return ICON_FA_CUBE" Meshes";
    }

    void MeshTab::Render(const gui::TextBoxWithFilter& filter)
    {
#ifdef IMGUI_ENABLED
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float detailPanelHeight = selectedMeshHash.has_value() ? 150.0f : 0.0f;
        float gridHeight = availableHeight - detailPanelHeight;

        // Grid view (top section)
        RenderGridView(filter, gridHeight);

        // Detail panel (bottom section) - only if something is selected
        if (selectedMeshHash.has_value())
        {
            ImGui::Separator();
            RenderDetailPanel();
        }
#endif
    }

    void MeshTab::RenderGridView(const gui::TextBoxWithFilter& filter, float height)
    {
#ifdef IMGUI_ENABLED
        ImGui::BeginChild("MeshGrid", ImVec2(0, height), false);

        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = ImGui::GetContentRegionAvail().x;
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, mesh] : ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
        {
            const std::string& meshName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(meshName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                // Highlight selected item
                bool isSelected = selectedMeshHash.has_value() && selectedMeshHash.value() == hash.get();
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
                }

                if (gui::Button button{ ICON_FA_CUBE, thumbnailSizeVec2 })
                {
                    // Toggle selection
                    if (isSelected)
                        selectedMeshHash.reset();
                    else
                        selectedMeshHash = hash.get();
                }

                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }

                gui::PayloadSource{ "MESH_HASH", hash.get() };

                gui::ShowSimpleHoverTooltip(meshName);
                gui::ThumbnailLabel(meshName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
#endif
    }

    void MeshTab::RenderDetailPanel()
    {
#ifdef IMGUI_ENABLED
        if (!selectedMeshHash.has_value())
            return;

        ImGui::BeginChild("MeshDetails", ImVec2(0, 0), true);

        size_t hash = selectedMeshHash.value();
        const std::string* meshName = ST<MagicResourceManager>::Get()->Editor_GetName(hash);

        if (!meshName)
        {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Mesh not found");
            ImGui::EndChild();
            return;
        }

        // Header with close button
        ImGui::Text(ICON_FA_CUBE " %s", meshName->c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        if (ImGui::SmallButton(ICON_FA_XMARK))
        {
            selectedMeshHash.reset();
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();

        // Check if mesh is loaded (without triggering a load)
        auto& container = ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceMesh>();
        ResourceMesh* mesh = const_cast<ResourceMesh*>(static_cast<const ResourceMesh*>(container.GetResource(hash)));

        if (!mesh || !mesh->IsLoaded())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Not loaded");
            ImGui::TextDisabled("This mesh is registered but not loaded into memory.");
            ImGui::TextDisabled("Drag to scene or use in a component to load it.");

            ImGui::Spacing();
            ImGui::Button(ICON_FA_HAND " Drag to Scene", ImVec2(-1, 0));
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
            {
                ImGui::SetDragDropPayload("MESH_HASH", &hash, sizeof(size_t));
                ImGui::Text("Mesh: %s", meshName->c_str());
                ImGui::EndDragDropSource();
            }

            ImGui::EndChild();
            return;
        }

        // Mesh info (only shown when loaded)
        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 250);

        // Left column - Mesh details
        ImGui::Text("Submeshes: %zu", mesh->handles.size());

        // Materials section
        ImGui::Spacing();
        ImGui::Text(ICON_FA_PALETTE " Materials (%zu):", mesh->defaultMaterialHashes.size());

        ImGui::BeginChild("MaterialsList", ImVec2(0, 80), true);
        for (size_t i = 0; i < mesh->defaultMaterialHashes.size(); ++i)
        {
            size_t matHash = mesh->defaultMaterialHashes[i];
            const std::string* matName = ST<MagicResourceManager>::Get()->Editor_GetName(matHash);

            ImGui::PushID(static_cast<int>(i));
            if (matName)
            {
                // Truncate long material names for display
                std::string displayName = *matName;
                if (displayName.length() > 35)
                    displayName = displayName.substr(0, 32) + "...";

                ImGui::Selectable(displayName.c_str(), false);

                // Drag source for material
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                {
                    ImGui::SetDragDropPayload("MATERIAL_HASH", &matHash, sizeof(size_t));
                    ImGui::Text("Material: %s", matName->c_str());
                    ImGui::EndDragDropSource();
                }

                // Tooltip with full name
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", matName->c_str());
                    ImGui::EndTooltip();
                }
            }
            else
            {
                ImGui::TextDisabled("[Hash: %zu]", matHash);
            }
            ImGui::PopID();
        }
        ImGui::EndChild();

        ImGui::NextColumn();

        // Right column - Actions
        ImGui::Text("Actions:");
        ImGui::Spacing();

        // Drag entire mesh with all materials
        ImGui::Button(ICON_FA_HAND " Drag to Scene", ImVec2(-1, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("MESH_HASH", &hash, sizeof(size_t));
            ImGui::Text("Mesh: %s", meshName->c_str());
            ImGui::Text("  + %zu materials", mesh->defaultMaterialHashes.size());
            ImGui::EndDragDropSource();
        }

        ImGui::Columns(1);

        ImGui::EndChild();
#endif
    }

}