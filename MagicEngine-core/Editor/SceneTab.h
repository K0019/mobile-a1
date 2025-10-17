#pragma once
#include "Editor/AssetBrowserCategories.h"


struct SceneTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};
