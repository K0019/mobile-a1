#include "Editor/MiscAssetTabs.h"
#include "Assets/AssetManager.h"
#include "Engine/PrefabManager.h"
#include "Editor/Import.h"
#include "Editor/EditorHistory.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/AssetBrowser.h"

#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeBasic.h"
#include "Engine/Events/EventsTypeEditor.h"

#ifdef GLFW
#include <Windows.h>
#include <shellapi.h>
#endif

namespace editor {

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

    void PrefabTab::Render(const gui::TextBoxWithFilter& filter)
    {
        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (size_t i{}; i < PrefabManager::AllPrefabs().size(); ++i)
        {
            const std::string& prefabName{ PrefabManager::AllPrefabs()[i] };
            if (!filter.PassFilter(prefabName))
                continue;

            gui::GridItem item{ grid.Item(prefabName) };

            if (gui::Button{ ICON_FA_CUBE, gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } })
            {
                ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(prefabName) };
                ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
                ST<EventsQueue>::Get()->AddEventForNextFrame(Events::EditorSelectEntity{ prefabEntity });
            }

            gui::PayloadSource{ "PREFAB", prefabName, std::string{ ICON_FA_CUBE + prefabName }.c_str() };

            if (gui::ItemContextMenu contextMenu{ prefabName.c_str() })
                if (gui::MenuItem(ICON_FA_TRASH" Delete"))
                {
                    CONSOLE_LOG(LEVEL_DEBUG) << "DELETE " << prefabName;
                    PrefabManager::DeletePrefab(prefabName);
                }
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

    void FontTab::Render([[maybe_unused]] const gui::TextBoxWithFilter& filter)
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

    void ShaderTab::Render([[maybe_unused]] const gui::TextBoxWithFilter& filter)
    {
        if (shaderNames.empty())
        {
            const std::unordered_set<std::string> allowedExtensions{ ".vert", ".frag", ".comp" };
            for (const auto& entry : std::filesystem::directory_iterator{ VFS::ConvertVirtualToPhysical(Filepaths::shadersSave) })
                if (allowedExtensions.find(entry.path().extension().string()) != allowedExtensions.end())
                    shaderNames.push_back(entry.path().filename().string());
        }

        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        for (const auto& shaderName : shaderNames)
        {
            gui::GridItem item{ grid.Item(shaderName) };

            gui::Button{ "##shader", gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } };
        }
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

    void ScriptTab::Render(const gui::TextBoxWithFilter& filter)
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
        gui::NewGridHelper grid{ AssetBrowser::THUMBNAIL_SIZE };

        int count{};
        for (const auto& filename : VFS::ListDirectory(Filepaths::scriptsSave))
        {
            if (VFS::GetExtension(filename) != ".lua")
                continue;
            if (!filter.PassFilter(filename))
                continue;

            gui::GridItem item{ grid.Item(filename) };

            if (gui::Button button{ ICON_FA_CODE"##script", gui::Vec2{ AssetBrowser::THUMBNAIL_SIZE, AssetBrowser::THUMBNAIL_SIZE } })
#ifdef GLFW
                ShellExecute(0, 0, VFS::ConvertVirtualToPhysical(VFS::JoinPath(Filepaths::scriptsSave, filename)).c_str(), 0, 0, SW_SHOW);
#else
                (void)0;
#endif
        }
    }

}
