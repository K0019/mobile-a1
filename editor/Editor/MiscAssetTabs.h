#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Editor/Containers/GUICollection.h"

namespace editor {

	struct PrefabTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;
	};

	struct ShaderTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;

	private:
		std::vector<std::string> shaderNames;
	};

	struct FontTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;
	};

	struct ScriptTab : public BaseAssetCategory
	{
		ScriptTab();

		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;

	private:
		gui::TextBoxWithBuffer<256> newScriptName;
	};

}
