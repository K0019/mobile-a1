#pragma once
#include "Editor/AssetBrowserCategories.h"

namespace editor {

	struct SceneTab : public BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void Render(const gui::TextBoxWithFilter& filter) override;
	};

}
