#pragma once
#include "AssetBrowserCategories.h"


struct SpriteTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct SoundTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	void RenderSoundContextMenu(std::string const& name, bool isGrouped);
};

struct PrefabTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};

struct SceneTab
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
};

struct FontTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};
