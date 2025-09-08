#pragma once
#include "AssetBrowserCategories.h"

struct PrefabTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct ShaderTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	std::vector<std::string> shaderNames;
};

struct FontTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct ScriptTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	char buffer[1024] = "NewScript";
};