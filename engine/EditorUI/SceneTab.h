#pragma once
#include "AssetBrowserCategories.h"


struct SceneTab
	: BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void Render() override;
};
