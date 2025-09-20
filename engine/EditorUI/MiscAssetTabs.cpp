#include "MiscAssetTabs.h"
#include "ResourceManager.h"
#include "ScriptManagement.h"
#include "CSScripting.h"
#include "PrefabWindow.h"
#include "Import.h"
#include "EditorGuiUtils.h"

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
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    float panelWidth = ImGui::GetContentRegionAvail().x;
    gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

    gui::SetStyleVar itemSpacing(gui::FLAG_STYLE_VAR::ITEM_SPACING, ImVec2(5, 5));
    gui::SetStyleVar framePadding(gui::FLAG_STYLE_VAR::FRAME_PADDING, ImVec2(2, 2));

    if (shaderNames.empty())
    {
        const std::unordered_set<std::string> allowedExtensions{ ".vert", ".frag", ".comp" };
        for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->shadersSave })
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

}


///////////////
/// Scripts ///
///////////////
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
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
    ImGui::InputText(" ", buffer, sizeof(buffer));
    if (ImGui::Button("Create Script") && ST<ScriptManager>::Get()->EnsureScriptsFolderExists())
    {
        CONSOLE_LOG(LEVEL_DEBUG) << "Name of Script: " << buffer << ".cs";

        if (!ST<ScriptManager>::Get()->CreateScript(buffer))
            CONSOLE_LOG(LEVEL_ERROR) << "Failed to create script: " << buffer;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        //CSharpScripts::CSScripting::ReloadAssembly();
    }
    ImGui::Separator();

    std::string path = ST<Filepaths>::Get()->scriptsSave;
    std::vector<std::string> scriptFiles;

    if (ST<ScriptManager>::Get()->EnsureScriptsFolderExists())
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".cs")
            {
                scriptFiles.push_back(entry.path().filename().string());
            }
        }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

    // Define the number of columns you want
    float thumbnailSize = THUMBNAIL_SIZE * 2;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    int columnsCount = static_cast<int>(availableWidth / thumbnailSize);  // Calculate number of columns

    for (size_t i = 0; i < scriptFiles.size(); ++i)
    {
        const std::string& scriptname = scriptFiles[i];
        if (!MatchesFilter(scriptname))
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        ImGui::BeginGroup(); // Group to keep each button and label together

        // Create the thumbnail (button)
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // Set the drag-and-drop payload (here, the file name)
            std::string sent_script = scriptname;
            ImGui::SetDragDropPayload("SCRIPT", sent_script.c_str(), (sent_script.size() + 1) * sizeof(char));
            ImGui::Text(ICON_FA_CODE);  // Display the icon during dragging
            ImGui::EndDragDropSource();
        }

        // Draw the icon button (thumbnail)
        if (ImGui::Button(ICON_FA_CODE, ImVec2(thumbnailSize, thumbnailSize)))
        {
            // Single button click if needed
        }

        // Check for double-click on the item (icon)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            // When double-clicked, load the script
            ST<ScriptManager>::Get()->OpenScript(scriptname);
        }

        // Script Name Label (showing truncated if it's too long)
        std::string displayName = scriptname;
        float maxWidth = thumbnailSize;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if (textSize.x > maxWidth)
        {
            // Truncate text if it's too wide
            while (textSize.x > maxWidth && displayName.length() > 3)
            {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }

        // Center the text label below the thumbnail
        float textX = (thumbnailSize - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());

        ImGui::EndGroup();

        ImGui::PopID();

        // Arrange items in columns
        if ((static_cast<int>(i) + 1) % columnsCount != 0)
        {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
        else
        {
            ImGui::NewLine();
        }
    }

    ImGui::PopStyleVar();
}
