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
    // Create new script text box & button
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

    // Reload scripts button
    gui::SameLine();
    if (gui::Button reloadButton{ "Reload" })
        ST<EventsQueue>::Get()->AddEventForThisFrame(Events::RequestReloadLuaScripts{});
    gui::Separator();

    // Grid list
    constexpr float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
    gui::GridHelper grid{ gui::GetAvailableContentRegion().x, THUMBNAIL_SIZE };
    gui::SetStyleVar itemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 5, 5 } };
    gui::SetStyleVar framePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 2, 2 } };

    int count{};
    for (const auto& filename : VFS::ListDirectory(Filepaths::scriptsSave))
    {
        if (VFS::GetExtension(filename) != ".lua")
            continue;

        {
            gui::SetID id{ count++ };
            gui::Group group{};

            if (gui::Button button{ ICON_FA_CODE"##script", gui::Vec2{ THUMBNAIL_SIZE, THUMBNAIL_SIZE } })
#ifdef GLFW
                ShellExecute(0, 0, VFS::ConvertVirtualToPhysical(VFS::JoinPath(Filepaths::scriptsSave, filename)).c_str(), 0, 0, SW_SHOW);
#else
                (void)0;
#endif

            // Name label
            gui::ThumbnailLabel(VFS::GetStem(filename), THUMBNAIL_SIZE);
        }

        grid.NextItem();
    }
}
