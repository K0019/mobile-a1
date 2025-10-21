#pragma once
#include "Editor/AssetBrowserCategories.h"

class MaterialTab : public editor::BaseAssetCategory
{
	const char* GetName() const final;
	const char* GetIdentifier() const final;
	void Render() final;
};

