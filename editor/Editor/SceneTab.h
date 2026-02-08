#pragma once
#include "Editor/AssetBrowserCategories.h"

namespace editor {

	class SceneTab : public GenericStringAssetTab
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		std::vector<std::string> GetItemList() const override;
		std::string GetDisplayName(const std::string& item) const override;
		void OnItemClicked(const std::string& item) override;

	private:
		static const AssetTabConfig config;
	};

}
