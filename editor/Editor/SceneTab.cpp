#include "Editor/SceneTab.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/SceneManagement.h"
#include "FilepathConstants.h"
#include "VFS/VFS.h"

namespace editor {

	const AssetTabConfig SceneTab::config = {
		.name = "Scenes",
		.identifier = ICON_FA_DIAMOND " Scenes",
		.icon = ICON_FA_DIAMOND,
		.payloadType = nullptr,  // No drag-drop for scenes
		.iconColor = {0.3f, 0.8f, 0.9f, 1.0f},  // Cyan
		.thumbnailType = ThumbnailCache::AssetType::Texture,
		.hasThumbnails = false
	};

	const AssetTabConfig& SceneTab::GetConfig() const
	{
		return config;
	}

	std::vector<std::string> SceneTab::GetItemList() const
	{
		std::vector<std::string> scenes;
		for (const auto& entry : VFS::ListDirectory(Filepaths::scenesSave))
		{
			if (VFS::GetExtension(entry) == ".scene")
				scenes.push_back(entry);
		}
		return scenes;
	}

	std::string SceneTab::GetDisplayName(const std::string& item) const
	{
		return VFS::GetStem(item);
	}

	void SceneTab::OnItemClicked(const std::string& item)
	{
		ST<SceneManager>::Get()->LoadScene(VFS::JoinPath(Filepaths::scenesSave, item));
	}

}
