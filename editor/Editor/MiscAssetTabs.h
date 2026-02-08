#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Editor/Containers/GUICollection.h"

namespace editor {

	class PrefabTab : public GenericStringAssetTab
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		std::vector<std::string> GetItemList() const override;
		void OnItemClicked(const std::string& item) override;
		void RenderContextMenuItems(const std::string& item) override;
		void RenderDetailPanelContent(const std::string& asset, const std::string& assetName) override;

	private:
		static const AssetTabConfig config;
	};

	class ShaderTab : public GenericStringAssetTab
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		std::vector<std::string> GetItemList() const override;

	private:
		static const AssetTabConfig config;
		mutable std::vector<std::string> cachedShaderNames;
	};

	struct FontTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;
	};

	class ScriptTab : public GenericStringAssetTab
	{
	public:
		ScriptTab();

	protected:
		const AssetTabConfig& GetConfig() const override;
		std::vector<std::string> GetItemList() const override;
		std::string GetDisplayName(const std::string& item) const override;
		void OnItemClicked(const std::string& item) override;
		void RenderToolbar() override;

	private:
		static const AssetTabConfig config;
		gui::TextBoxWithBuffer<256> newScriptName;
	};

}
