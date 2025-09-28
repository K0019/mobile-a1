#include "SpriteTab.h"
#include "ResourceManager.h"
#include "EditorGuiUtils.h"

const char* SpriteTab::GetName() const
{
    return "Sprites";
}

const char* SpriteTab::GetIdentifier() const
{
    return ICON_FA_IMAGE" Sprites";
}

void SpriteTab::Render()
{
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    for (size_t i = 0; i < ResourceManagerOld::GetSpriteCount(); ++i)
    {
        const Sprite& sprite = ResourceManagerOld::GetSprite(i);
        bool isValid = sprite.textureID != ResourceManagerOld::INVALID_TEXTURE_ID;
        if (!isValid)
            continue;

        if (!editor::MatchesFilter(sprite.name))
        {
            continue;
        }

        gui::SetID thumbnailID(static_cast<int>(i));

        {
            gui::Group group;

            //const Texture& tex = ResourceManagerOld::GetTexture(sprite.textureName);
            ImGui::ImageButton("##sprite",
                0,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                ImVec2(sprite.texCoords.z, sprite.texCoords.w));

            //gui::PayloadSource<size_t>("SPRITE_ID", i, sprite.name.c_str());
            gui::PayloadSourceWithImageTooltip<size_t>("SPRITE_ID", i,0, ImVec2(THUMBNAIL_SIZE / 2, THUMBNAIL_SIZE / 2));

            // Hover tooltip
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("Name: %s", sprite.name.c_str());
                ImGui::Text("Size: %dx%d", sprite.width, sprite.height);
                ImGui::Text("Texture: %s", sprite.textureName.c_str());

                // Add animation usage info to tooltip
                bool isUsed = false;
                int useCount = 0;
                for (const auto& [_, anim] : ResourceManagerOld::GetAnimations())
                {
                    for (const auto& frame : anim.frames)
                    {
                        if (frame.spriteID == i)
                        {
                            isUsed = true;
                            useCount++;
                            break;
                        }
                    }
                }
                if (isUsed)
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                        "Used in %d animation(s)", useCount);
                }

                ImGui::EndTooltip();
            }

            // Context menu with proper ID
            if (ImGui::BeginPopupContextItem("sprite_context"))
            {
                static char renameBuffer[256];
                static bool isRenaming = false;

                // Check if sprite is used in any animations
                bool isSpriteUsed = false;
                for (const auto& [_, anim] : ResourceManagerOld::GetAnimations())
                {
                    if (std::any_of(anim.frames.begin(), anim.frames.end(),
                        [i](const FrameData& frame) { return frame.spriteID == i; }))
                    {
                        isSpriteUsed = true;
                        break;
                    }
                }

                if (isRenaming)
                {
                    if (ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer),
                        ImGuiInputTextFlags_EnterReturnsTrue))
                    {
                        ResourceManagerOld::RenameSprite(i, renameBuffer);
                        isRenaming = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("OK"))
                    {
                        ResourceManagerOld::RenameSprite(i, renameBuffer);
                        isRenaming = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("Cancel"))
                    {
                        isRenaming = false;
                        ImGui::CloseCurrentPopup();
                    }
                }
                else
                {
                    if (ImGui::MenuItem(ICON_FA_PEN" Rename"))
                    {
                        strncpy_s(renameBuffer, sprite.name.c_str(), sizeof(renameBuffer) - 1);
                        isRenaming = true;
                    }

                    // Disable delete option if sprite is used in animations
                    if (isSpriteUsed)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                        ImGui::MenuItem(ICON_FA_TRASH" Delete (Used in Animations)", nullptr, false, false);
                        if (ImGui::IsItemHovered())
                        {
                            ImGui::SetTooltip("Cannot delete sprite while it's used in animations");
                        }
                        ImGui::PopStyleColor();
                    }
                    else
                    {
                        if (ImGui::MenuItem(ICON_FA_TRASH" Delete"))
                        {
                            ResourceManagerOld::DeleteSprite(i);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // Add info about sprite usage
                    if (isSpriteUsed)
                    {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Used in Animations:");
                        for (const auto& [nameHash, anim] : ResourceManagerOld::GetAnimations())
                        {
                            bool usedInThisAnim = std::any_of(anim.frames.begin(), anim.frames.end(),
                                [i](const FrameData& frame) { return frame.spriteID == i; });
                            if (usedInThisAnim)
                            {
                                ImGui::BulletText("%s", ResourceManagerOld::GetResourceName(nameHash).c_str());
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }

            gui::ThumbnailLabel(sprite.name, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
#endif

}