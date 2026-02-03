#include "Editor/TextureTab.h"
#include "Editor/AssetBrowser.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/ThumbnailCache.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

    const char* TextureTab::GetName() const
    {
        return "Textures";
    }

    const char* TextureTab::GetIdentifier() const
    {
        return ICON_FA_IMAGE" Textures";
    }

    void TextureTab::Render(const gui::TextBoxWithFilter& filter)
    {
#ifdef IMGUI_ENABLED
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float detailPanelHeight = selectedTextureHash.has_value() ? 120.0f : 0.0f;
        float gridHeight = availableHeight - detailPanelHeight;

        RenderGridView(filter, gridHeight);

        if (selectedTextureHash.has_value())
        {
            ImGui::Separator();
            RenderDetailPanel();
        }
#endif
    }

    void TextureTab::RenderGridView(const gui::TextBoxWithFilter& filter, float height)
    {
#ifdef IMGUI_ENABLED
        ImGui::BeginChild("TextureGrid", ImVec2(0, height), false);

        float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
        gui::Vec2 thumbnailSizeVec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE };
        float panelWidth = ImGui::GetContentRegionAvail().x;
        gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        int count{};
        for (const auto& [hash, textureRef] : ST<AssetManager>::Get()->Editor_GetContainer<ResourceTexture>().Editor_GetAllResources())
        {
            const std::string& textureName{ *ST<AssetManager>::Get()->Editor_GetName(hash) };
            if (!filter.PassFilter(textureName))
                continue;

            {
                gui::SetID id{ count++ };
                gui::Group group;

                bool isSelected = selectedTextureHash.has_value() && selectedTextureHash.value() == hash.get();
                if (isSelected)
                {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
                }

                // Try to get thumbnail from cache
                uint64_t thumbId = ThumbnailCache::Get().GetThumbnail(
                    hash.get(), ThumbnailCache::AssetType::Texture, textureName);

                bool clicked = false;
                if (thumbId != 0)
                {
                    // Display actual thumbnail (ImTextureID is ImU64)
                    clicked = ImGui::ImageButton(
                        ("##tex" + std::to_string(count)).c_str(),
                        static_cast<ImTextureID>(thumbId),
                        ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
                }
                else
                {
                    // Fall back to icon
                    clicked = ImGui::Button(
                        (std::string(ICON_FA_IMAGE) + "##" + std::to_string(count)).c_str(),
                        ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
                }

                if (clicked)
                {
                    if (isSelected)
                        selectedTextureHash.reset();
                    else
                        selectedTextureHash = hash.get();
                }

                if (isSelected)
                {
                    ImGui::PopStyleColor();
                }

                gui::PayloadSource{ "TEXTURE_HASH", hash.get() };

                gui::ShowSimpleHoverTooltip(textureName);
                gui::ThumbnailLabel(textureName, THUMBNAIL_SIZE);
            }

            grid.NextItem();
        }

        ImGui::PopStyleVar(2);
        ImGui::EndChild();
#endif
    }

    void TextureTab::RenderDetailPanel()
    {
#ifdef IMGUI_ENABLED
        if (!selectedTextureHash.has_value())
            return;

        ImGui::BeginChild("TextureDetails", ImVec2(0, 0), true);

        size_t hash = selectedTextureHash.value();
        const std::string* texName = ST<AssetManager>::Get()->Editor_GetName(hash);

        if (!texName)
        {
            ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "Texture not found");
            ImGui::EndChild();
            return;
        }

        // Header with close button
        ImGui::Text(ICON_FA_IMAGE " %s", texName->c_str());
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
        if (ImGui::SmallButton(ICON_FA_XMARK))
        {
            selectedTextureHash.reset();
            ImGui::EndChild();
            return;
        }

        ImGui::Separator();

        ImGui::Columns(2, nullptr, false);

        // Left column - Texture icon placeholder
        ImGui::Button(ICON_FA_IMAGE, ImVec2(80, 80));

        ImGui::NextColumn();

        // Right column - Actions
        ImGui::Text("Actions:");
        ImGui::Spacing();

        ImGui::Button(ICON_FA_HAND " Drag to assign", ImVec2(-1, 0));
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            ImGui::SetDragDropPayload("TEXTURE_HASH", &hash, sizeof(size_t));
            ImGui::Text("Texture: %s", texName->c_str());
            ImGui::EndDragDropSource();
        }

        ImGui::Columns(1);

        ImGui::EndChild();
#endif
    }

}
