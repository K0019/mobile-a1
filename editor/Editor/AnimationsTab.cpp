#include "Editor/AnimationsTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Resources/Types/ResourceTypesGraphics.h"

namespace editor
{
    const char* AnimationsTab::GetName() const
    {
        return "Animations";
    }

    const char* AnimationsTab::GetIdentifier() const
    {
        return ICON_FA_PERSON_RUNNING" Animations";
    }

    void AnimationsTab::Render(const gui::TextBoxWithFilter& filter)
    {
#ifdef IMGUI_ENABLED
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float detailPanelHeight = selectedAnimationHash.has_value() ? 150.0f : 0.0f;
        float gridHeight = availableHeight - detailPanelHeight;

        RenderGridView(filter, gridHeight);

        if (selectedAnimationHash.has_value())
        {
            ImGui::Separator();
            RenderDetailPanel();
        }
#endif
    }

    void AnimationsTab::RenderGridView(const gui::TextBoxWithFilter& filter, float height)
    {
#ifdef IMGUI_ENABLED
        ImGui::BeginChild("AnimationGrid", ImVec2(0, height), false);

        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = ImGui::GetContentRegionAvail().x;
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, animRef] : ST<MagicResourceManager>::Get()->Editor_GetContainer<ResourceAnimation>().Editor_GetAllResources())
        {
            const std::string& animName{ *ST<MagicResourceManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(animName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                bool isSelected = selectedAnimationHash.has_value() && selectedAnimationHash.value() == hash.get();
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
                }

                if (gui::Button button{ ICON_FA_PERSON_RUNNING, thumbnailSizeVec2 })
                {
                    if (isSelected)
                        selectedAnimationHash.reset();
                    else
                        selectedAnimationHash = hash.get();
                }

                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }

                gui::PayloadSource{ "ANIMATION_HASH", hash.get() };

                gui::ShowSimpleHoverTooltip(animName);
                gui::ThumbnailLabel(animName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
#endif
    }

    void AnimationsTab::RenderDetailPanel()
    {
#ifdef IMGUI_ENABLED
        if (!selectedAnimationHash.has_value())
            return;

        ImGui::BeginChild("AnimationDetails", ImVec2(0, 0), true);

        size_t hash = selectedAnimationHash.value();
        const std::string* animName = ST<MagicResourceManager>::Get()->Editor_GetName(hash);

        if (!animName)
        {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Animation not found");
            ImGui::EndChild();
            return;
        }

        // Header with close button
        ImGui::Text(ICON_FA_PERSON_RUNNING " %s", animName->c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        if (ImGui::SmallButton(ICON_FA_XMARK))
        {
            selectedAnimationHash.reset();
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);
        ImGui::SetColumnWidth(0, 300);

        // Find meshes with similar folder path (likely compatible)
        ImGui::Text(ICON_FA_CUBE " Related meshes:");
        ImGui::BeginChild("RelatedMeshList", ImVec2(0, 80), true);

        // Extract folder hint from animation name
        std::string animFolder;
        if (animName)
        {
            // Try to find a common prefix pattern
            // e.g., "mc_animatedIdle" -> look for "mc_animated" meshes
            std::string name = *animName;
            // Find where the animation action name starts (usually uppercase or after known prefixes)
            for (size_t i = 3; i < name.length(); ++i)
            {
                if (std::isupper(name[i]) || name[i] == '_')
                {
                    animFolder = name.substr(0, i);
                    break;
                }
            }
        }

        int relatedCount = 0;
        if (!animFolder.empty())
        {
            for (const auto& [meshHash, meshRef] : ST<MagicResourceManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
            {
                const std::string* meshName = ST<MagicResourceManager>::Get()->Editor_GetName(meshHash);
                if (meshName && meshName->find(animFolder) != std::string::npos)
                {
                    ImGui::Selectable(meshName->c_str(), false);
                    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
                    {
                        size_t dragHash = meshHash.get();
                        ImGui::SetDragDropPayload("MESH_HASH", &dragHash, sizeof(size_t));
                        ImGui::Text("Mesh: %s", meshName->c_str());
                        ImGui::EndDragDropSource();
                    }
                    relatedCount++;
                }
            }
        }

        if (relatedCount == 0)
        {
            ImGui::TextDisabled("No related meshes found");
        }

        ImGui::EndChild();

        ImGui::NextColumn();

        // Right column - Actions
        ImGui::Text("Actions:");
        ImGui::Spacing();

        ImGui::Button(ICON_FA_HAND " Drag to assign", ImVec2(-1, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("ANIMATION_HASH", &hash, sizeof(size_t));
            ImGui::Text("Animation: %s", animName->c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::Columns(1);

        ImGui::EndChild();
#endif
    }
}