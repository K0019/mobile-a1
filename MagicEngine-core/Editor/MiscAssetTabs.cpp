#include "Editor/MiscAssetTabs.h"
#include "Engine/Resources/ResourceManager.h"
#include "Editor/PrefabWindow.h"
#include "Editor/Import.h"
#include "Editor/EditorHistory.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/AssetBrowser.h"

#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"

#ifdef GLFW
#include <Windows.h>
#include <shellapi.h>
#endif

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
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x; // random offset
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    std::string prefabName = "";
    for (size_t i = 0; i < PrefabManager::AllPrefabs().size(); ++i)
    {
        prefabName = PrefabManager::AllPrefabs()[i];
        if (!editor::MatchesFilter(prefabName))
        {
            continue;
        }

        {
            gui::SetID setid(static_cast<int>(i));
            gui::Group group;

            bool clicked = ImGui::Button(ICON_FA_CUBE,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE + 10));

            gui::ShowSimpleHoverTooltip(prefabName);
            gui::PayloadSource<std::string>{ "PREFAB", prefabName.c_str(), std::string{ ICON_FA_CUBE + prefabName }.c_str() };

            if (clicked)
            {
                ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(prefabName) };
                ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
            }

            // Name label
            gui::ThumbnailLabel(prefabName, THUMBNAIL_SIZE);
        }

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
#endif
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
    /*const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();

    gui::Child child("FontTable", ImVec2(0.0f, 0.0f));
    gui::TextFormatted("Loaded Fonts");
    gui::Separator();

    for (const auto& [atlasName, atlas] : fontAtlases)
    {
        gui::TextFormatted("%s", atlasName.c_str());
    }*/
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
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    if (shaderNames.empty())
    {
        const std::unordered_set<std::string> allowedExtensions{ ".vert", ".frag", ".comp" };
        for (const auto& entry : std::filesystem::directory_iterator{ Filepaths::shadersSave })
            if (allowedExtensions.find(entry.path().extension().string()) != allowedExtensions.end())
                shaderNames.push_back(entry.path().filename().string());
    }

    int count{};
    for (const auto& shaderName : shaderNames)
    {
        {
            gui::SetID id{ count++ };
            gui::Group group;

            gui::Button button("##shader", gui::Vec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE });

            // Name label
            gui::ThumbnailLabel(shaderName, THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
#endif
}


///////////////
/// Scripts ///
///////////////
ScriptTab::ScriptTab()
    : newScriptName{ "New Script Name" }
{
}

const char* ScriptTab::GetName() const
{
    return "Scripts";
}

const char* ScriptTab::GetIdentifier() const
{
    return ICON_FA_CODE" Scripts";
}

void ScriptTab::Render()
{
#ifdef IMGUI_ENABLED
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    newScriptName.Draw();
    {
        gui::Disabled buttonDisable{ *newScriptName.GetBufferPtr() == '\0' };
        if (gui::Button scriptButton{ "Create Script" })
        {
            const std::string scriptFilepath{ VFS::JoinPath(Filepaths::scriptsSave, newScriptName.GetBuffer() + ".lua") };
            if (!VFS::FileExists(scriptFilepath))
            {
                // Create the file
                if (!VFS::WriteFile(scriptFilepath, "function update(entity)\n\t\nend"))
                    CONSOLE_LOG(LEVEL_ERROR) << "Failed to generate new script file";
#ifdef GLFW
                // Then open the file
                else
                {
                    ShellExecute(0, 0, VFS::ConvertVirtualToPhysical(scriptFilepath).c_str(), 0, 0, SW_SHOW);
                    CONSOLE_LOG(LEVEL_INFO) << "Created new script file " << newScriptName.GetBuffer();
                }
#endif
            }
        }
    }
    gui::SameLine();
    if (gui::Button reloadButton{ "Reload" })
        ST<EventsQueue>::Get()->AddEventForThisFrame(Events::RequestReloadLuaScripts{});
    gui::Separator();

    //std::string path = Filepaths::scriptsSave;
    //std::vector<std::string> scriptFiles;

    //if (ST<ScriptManager>::Get()->EnsureScriptsFolderExists())
    //{
    //    for (const auto& entry : std::filesystem::directory_iterator(path))
    //    {
    //        if (entry.is_regular_file() && entry.path().extension() == ".cs")
    //        {
    //            scriptFiles.push_back(entry.path().filename().string());
    //        }
    //    }
    //}
    //ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

    //// Define the number of columns you want
    //float thumbnailSize = THUMBNAIL_SIZE * 2;
    //float availableWidth = ImGui::GetContentRegionAvail().x;
    //int columnsCount = static_cast<int>(availableWidth / thumbnailSize);  // Calculate number of columns

    //for (size_t i = 0; i < scriptFiles.size(); ++i)
    //{
    //    const std::string& scriptname = scriptFiles[i];
    //    if (!editor::MatchesFilter(scriptname))
    //    {
    //        continue;
    //    }

    //    ImGui::PushID(static_cast<int>(i));

    //    ImGui::BeginGroup(); // Group to keep each button and label together

    //    // Create the thumbnail (button)
    //    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
    //    {
    //        // Set the drag-and-drop payload (here, the file name)
    //        std::string sent_script = scriptname;
    //        ImGui::SetDragDropPayload("SCRIPT", sent_script.c_str(), (sent_script.size() + 1) * sizeof(char));
    //        ImGui::Text(ICON_FA_CODE);  // Display the icon during dragging
    //        ImGui::EndDragDropSource();
    //    }

    //    // Draw the icon button (thumbnail)
    //    if (ImGui::Button(ICON_FA_CODE, ImVec2(thumbnailSize, thumbnailSize)))
    //    {
    //        // Single button click if needed
    //    }

    //    // Check for double-click on the item (icon)
    //    if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
    //    {
    //        // When double-clicked, load the script
    //        ST<ScriptManager>::Get()->OpenScript(scriptname);
    //    }

    //    // Script Name Label (showing truncated if it's too long)
    //    std::string displayName = scriptname;
    //    float maxWidth = thumbnailSize;
    //    ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
    //    if (textSize.x > maxWidth)
    //    {
    //        // Truncate text if it's too wide
    //        while (textSize.x > maxWidth && displayName.length() > 3)
    //        {
    //            displayName = displayName.substr(0, displayName.length() - 4) + "...";
    //            textSize = ImGui::CalcTextSize(displayName.c_str());
    //        }
    //    }

    //    // Center the text label below the thumbnail
    //    float textX = (thumbnailSize - textSize.x) * 0.5f;
    //    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
    //    ImGui::TextUnformatted(displayName.c_str());

    //    ImGui::EndGroup();

    //    ImGui::PopID();

    //    // Arrange items in columns
    //    if ((static_cast<int>(i) + 1) % columnsCount != 0)
    //    {
    //        ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
    //    }
    //    else
    //    {
    //        ImGui::NewLine();
    //    }
    //}

    //ImGui::PopStyleVar();
#endif
}
