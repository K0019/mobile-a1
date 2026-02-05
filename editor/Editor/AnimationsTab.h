#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class AnimationsTab : public GenericAssetTab<ResourceAnimation>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderDetailPanelContent(size_t hash, const std::string& name) override;

	private:
		static const AssetTabConfig config;
	};

}
