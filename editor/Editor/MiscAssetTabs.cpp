#include "Editor/MiscAssetTabs.h"
#include "Assets/AssetManager.h"
#include "Engine/PrefabManager.h"
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

namespace editor {

	///////////////
	/// Prefabs ///
	///////////////
	const AssetTabConfig PrefabTab::config = {
		.name = "Prefabs",
		.identifier = ICON_FA_CUBE " Prefabs",
		.icon = ICON_FA_CUBE,
		.payloadType = "PREFAB",
		.iconColor = {0.6f, 0.8f, 1.0f, 1.0f},  // Light blue
		.thumbnailType = ThumbnailCache::AssetType::Texture,
		.hasThumbnails = false
	};

	const AssetTabConfig& PrefabTab::GetConfig() const
	{
		return config;
	}

	std::vector<std::string> PrefabTab::GetItemList() const
	{
		return PrefabManager::AllPrefabs();
	}

	void PrefabTab::OnItemClicked(const std::string& item)
	{
		// Don't spawn on click - user can drag or use detail panel
	}

	void PrefabTab::RenderContextMenuItems(const std::string& item)
	{
#ifdef IMGUI_ENABLED
		if (ImGui::MenuItem(ICON_FA_PLUS " Spawn"))
		{
			ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(item) };
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
		}
		if (ImGui::MenuItem(ICON_FA_TRASH " Delete"))
			PrefabManager::DeletePrefab(item);
#endif
	}

	void PrefabTab::RenderDetailPanelContent(const std::string& asset, [[maybe_unused]] const std::string& assetName)
	{
#ifdef IMGUI_ENABLED
		if (ImGui::Button(ICON_FA_PLUS " Spawn Prefab", ImVec2(-1, 0)))
		{
			ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(asset) };
			ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
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
		return ICON_FA_FONT " Fonts";
	}

	void FontTab::Render([[maybe_unused]] const gui::TextBoxWithFilter& filter)
	{
		// Font rendering not implemented
	}

	///////////////
	/// Shaders ///
	///////////////
	const AssetTabConfig ShaderTab::config = {
		.name = "Shaders",
		.identifier = ICON_FA_FILE_CODE " Shaders",
		.icon = ICON_FA_FILE_CODE,
		.payloadType = nullptr,  // No drag-drop for shaders
		.iconColor = {0.8f, 0.5f, 0.8f, 1.0f},  // Purple
		.thumbnailType = ThumbnailCache::AssetType::Texture,
		.hasThumbnails = false
	};

	const AssetTabConfig& ShaderTab::GetConfig() const
	{
		return config;
	}

	std::vector<std::string> ShaderTab::GetItemList() const
	{
		if (cachedShaderNames.empty())
		{
			const std::unordered_set<std::string> allowedExtensions{ ".vert", ".frag", ".comp" };
			for (const auto& entry : std::filesystem::directory_iterator{ VFS::ConvertVirtualToPhysical(Filepaths::shadersSave) })
				if (allowedExtensions.find(entry.path().extension().string()) != allowedExtensions.end())
					cachedShaderNames.push_back(entry.path().filename().string());
		}
		return cachedShaderNames;
	}

	///////////////
	/// Scripts ///
	///////////////
	const AssetTabConfig ScriptTab::config = {
		.name = "Scripts",
		.identifier = ICON_FA_CODE " Scripts",
		.icon = ICON_FA_CODE,
		.payloadType = "SCRIPT_HASH",
		.iconColor = {0.4f, 0.9f, 0.4f, 1.0f},  // Green
		.thumbnailType = ThumbnailCache::AssetType::Texture,
		.hasThumbnails = false
	};

	ScriptTab::ScriptTab()
		: newScriptName{ "New Script Name" }
	{
	}

	const AssetTabConfig& ScriptTab::GetConfig() const
	{
		return config;
	}

	std::vector<std::string> ScriptTab::GetItemList() const
	{
		std::vector<std::string> scripts;
		for (const auto& filename : VFS::ListDirectory(Filepaths::scriptsSave))
		{
			if (VFS::GetExtension(filename) == ".lua")
				scripts.push_back(filename);
		}
		return scripts;
	}

	std::string ScriptTab::GetDisplayName(const std::string& item) const
	{
		return VFS::GetStem(item);
	}

	void ScriptTab::OnItemClicked(const std::string& item)
	{
#ifdef GLFW
		ShellExecute(0, 0, VFS::ConvertVirtualToPhysical(VFS::JoinPath(Filepaths::scriptsSave, item)).c_str(), 0, 0, SW_SHOW);
#endif
	}

	void ScriptTab::RenderToolbar()
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
					if (!VFS::WriteFile(scriptFilepath, "function update(entity)\n\t\nend"))
						CONSOLE_LOG(LEVEL_ERROR) << "Failed to generate new script file";
#ifdef GLFW
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
	}

}
