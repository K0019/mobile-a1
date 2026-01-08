#pragma once
#include "Editor/AssetBrowserCategories.h"


namespace editor
{
	class AnimationsTab : public editor::BaseAssetCategory
	{
		const char* GetName() const final;
		const char* GetIdentifier() const final;
		void Render(const gui::TextBoxWithFilter& filter) final;
	};
}