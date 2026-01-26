#pragma once
#include "Editor/AssetBrowserCategories.h"

namespace editor {

	class GameTab : public BaseAssetCategory
	{
		const char* GetName() const final;
		const char* GetIdentifier() const final;
		void Render(const gui::TextBoxWithFilter& filter) final;
	};

}
