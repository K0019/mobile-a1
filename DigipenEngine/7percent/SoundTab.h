#pragma once
#include "AssetBrowserCategories.h"

struct SoundTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;

private:
	void RenderSoundContextMenu(std::string const& name, bool isGrouped);
};
