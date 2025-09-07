#include "MiscAssetTabs.h"
#include "ResourceManager.h"
#include "PrefabWindow.h"
#include "SceneManagement.h"
#include "Import.h"
#include "EditorGuiUtils.h"

///////////////
/// Sprites ///
///////////////
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
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    for (size_t i = 0; i < ResourceManager::GetSpriteCount(); ++i)
    {
        const Sprite& sprite = ResourceManager::GetSprite(i);
        bool isValid = sprite.textureID != ResourceManager::INVALID_TEXTURE_ID;
        if (!isValid)
            continue;

        if (!MatchesFilter(sprite.name))
        {
            continue;
        }

        gui::SetID thumbnailID(static_cast<int>(i));

        {
            gui::Group group;

            const Texture& tex = ResourceManager::GetTexture(sprite.textureName);
            ImGui::ImageButton("##sprite",
                tex.ImGui_handle,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                ImVec2(sprite.texCoords.z, sprite.texCoords.w));

            //gui::PayloadSource<size_t>("SPRITE_ID", i, sprite.name.c_str());
            gui::PayloadSourceWithImageTooltip<size_t>("SPRITE_ID", i, tex.ImGui_handle, ImVec2(THUMBNAIL_SIZE / 2, THUMBNAIL_SIZE / 2));

            gui::ThumbnailLabel(sprite.name, THUMBNAIL_SIZE);

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
                for (const auto& [_, anim] : ResourceManager::GetAnimations())
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
                for (const auto& [_, anim] : ResourceManager::GetAnimations())
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
                        ResourceManager::RenameSprite(i, renameBuffer);
                        isRenaming = false;
                    }
                    ImGui::SameLine();
                    if (ImGui::Button("OK"))
                    {
                        ResourceManager::RenameSprite(i, renameBuffer);
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
                            ResourceManager::DeleteSprite(i);
                            ImGui::CloseCurrentPopup();
                        }
                    }

                    // Add info about sprite usage
                    if (isSpriteUsed)
                    {
                        ImGui::Separator();
                        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Used in Animations:");
                        for (const auto& [nameHash, anim] : ResourceManager::GetAnimations())
                        {
                            bool usedInThisAnim = std::any_of(anim.frames.begin(), anim.frames.end(),
                                [i](const FrameData& frame) { return frame.spriteID == i; });
                            if (usedInThisAnim)
                            {
                                ImGui::BulletText("%s", ResourceManager::GetResourceName(nameHash).c_str());
                            }
                        }
                    }
                }
                ImGui::EndPopup();
            }
        }

        grid.NextItem();
    }

}

///////////////
/// Sounds  ///
///////////////
const char* SoundTab::GetName() const
{
    return "Sounds";
}

const char* SoundTab::GetIdentifier() const
{
    return ICON_FA_MICROPHONE" Sounds";
}

void SoundTab::Render()
{
    ImGui::BeginChild("SoundTable", ImVec2(0.0f, 0.0f), true);

    std::set<std::string> singleSoundNames = ST<AudioManager>::Get()->GetSingleSoundNames();
    std::map<std::string, std::set<std::string>> groupedSoundNames = ST<AudioManager>::Get()->GetGroupedSoundNames();

    ImGui::Columns(2, nullptr, true);

    // Left column for single sounds
    ImGui::Text("Single Sounds");

    // Iterate all single sound names
    for (std::string const& name : singleSoundNames)
    {
        // Create Button
        if (ImGui::Selectable(name.c_str()))
        {
            if (ST<AudioManager>::Get()->IsSoundPlaying(name))
            {
                ST<AudioManager>::Get()->StopSound(name);
            }
            else
            {
                ST<AudioManager>::Get()->StartSingleSound(name, false);
            }
        }

        // Drag-drop source
        gui::PayloadSource("SOUND", name, name.c_str());

        // Context menu
        RenderSoundContextMenu(name, false);
    }

    // Right column for grouped sounds
    ImGui::NextColumn();
    ImGui::Text("Grouped Sounds");
    ImGui::Separator();

    // Iterate all grouped sound names
    for (std::pair<std::string, std::set<std::string>> const& group : groupedSoundNames)
    {
        if (ImGui::TreeNode(group.first.c_str()))
        {
            ImGui::Indent(55.0f);
            for (std::string const& name : group.second)
            {
                // Create Button
                if (ImGui::Selectable(name.c_str()))
                {
                    if (ST<AudioManager>::Get()->IsSoundPlaying(name))
                    {
                        ST<AudioManager>::Get()->StopSound(name);
                    }
                    else
                    {
                        ST<AudioManager>::Get()->StartSpecificGroupedSound(name, false);
                    }
                }

                // Single sound drag-drop source
                gui::PayloadSource("SOUND", name, name.c_str());

                // Context menu
                RenderSoundContextMenu(name, true);
            }
            ImGui::Unindent(55.0f);
            ImGui::TreePop();
        }

        // Grouped sound drag-drop source
        gui::PayloadSource("SOUND", group.first, group.first.c_str());
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void SoundTab::RenderSoundContextMenu(std::string const& name, bool isGrouped)
{
    if (ImGui::BeginPopupContextItem(("Delete##" + name).c_str()))
    {
        if (ImGui::MenuItem("Delete"))
        {
            ST<AudioManager>::Get()->DeleteSound(name, isGrouped);
        }
        ImGui::EndPopup();
    }
}

///////////////
/// Prefabs ///
///////////////
const char* PrefabTab::GetName() const
{
    return "Prefabs";
}

const char* PrefabTab::GetIdentifier() const
{
    return ICON_FA_CUBE" Prefabs";
}

void PrefabTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    std::string prefabName = "";
    for (size_t i = 0; i < PrefabManager::AllPrefabs().size(); ++i)
    {
        prefabName = PrefabManager::AllPrefabs()[i];
        if (!MatchesFilter(prefabName))
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        {
            gui::Group group;

            bool clicked = ImGui::Button(ICON_FA_CUBE,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE + 10));

            gui::ShowSimpleHoverTooltip(prefabName);
            gui::PayloadSource<std::string>("PREFAB", prefabName.c_str(), std::string{ ICON_FA_CUBE + prefabName}.c_str());

            if (clicked)
            {
                ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(prefabName) };
                ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
            }

            // Name label
            gui::ThumbnailLabel(prefabName, THUMBNAIL_SIZE);
        }

        ImGui::PopID();

        if (ImGui::BeginPopupContextItem(prefabName.c_str()))
        {
            if (ImGui::MenuItem(ICON_FA_TRASH" Delete"))
            {
                CONSOLE_LOG_EXPLICIT("DELETE " + prefabName, LogLevel::LEVEL_DEBUG);
                PrefabManager::DeletePrefab(prefabName);
            }
            if (ImGui::MenuItem(ICON_FA_CLONE" Duplicate"))
            {
                CONSOLE_LOG_EXPLICIT("CLONE " + prefabName, LogLevel::LEVEL_DEBUG);
            }
            ImGui::EndPopup();
        }

        grid.NextItem();
    }
}

///////////////
///  Fonts  ///
///////////////
const char* FontTab::GetName() const
{
    return "Fonts";
}

const char* FontTab::GetIdentifier() const
{
    return ICON_FA_FONT" Fonts";
}

void FontTab::Render()
{
    const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();

    ImGui::BeginChild("FontTable", ImVec2(0.0f, 0.0f), true);
    ImGui::Text("Loaded Fonts");
    ImGui::Separator();

    for (const auto& [atlasName, atlas] : fontAtlases)
    {
        ImGui::Text("%s", atlasName.c_str());
    }

    ImGui::EndChild();
}

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
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    int count{};
    for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->scenesSave })
    {
        if (!MatchesFilter(entry.path().string()))
            continue;
        if (entry.path().extension() != ".scene")
            continue;

        ImGui::PushID(count++);

        ImGui::BeginGroup();

        if (ImGui::Button("##scene", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
        {
            ST<SceneManager>::Get()->LoadScene(entry.path());
        }

        // Name label
        std::string displayName = entry.path().stem().string();
        gui::ThumbnailLabel(displayName, THUMBNAIL_SIZE);
        ImGui::EndGroup();

        ImGui::PopID();

        grid.NextItem();
    }
}

///////////////
/// Shaders ///
///////////////
const char* ShaderTab::GetName() const
{
    return "Shaders";
}

const char* ShaderTab::GetIdentifier() const
{
    return ICON_FA_FILE_CODE" Shaders";
}

void ShaderTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

    static std::vector<std::string> shaderNames;
    if (shaderNames.empty())
    {
        for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->shadersSave })
            shaderNames.push_back(entry.path().filename().string());
    }

    int count{};
    for (const auto& shaderName : shaderNames)
    {
        gui::SetID id{ count++ };

        ImGui::BeginGroup();

        ImGui::Button("##shader", gui::Vec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE });

        // Name label
        gui::ThumbnailLabel(shaderName, THUMBNAIL_SIZE);
        ImGui::EndGroup();

        grid.NextItem();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleVar();
}
