#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Editor/Containers/GUICollection.h"

struct PrefabTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct ShaderTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	std::vector<std::string> shaderNames;
};

struct FontTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct ScriptTab
	: editor::BaseAssetCategory
{
	ScriptTab();

	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	gui::TextBoxWithBuffer<256> newScriptName;
};