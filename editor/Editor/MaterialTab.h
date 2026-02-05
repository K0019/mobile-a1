#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class MaterialTab : public GenericAssetTab<ResourceMaterial>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderContextMenuItems(size_t hash) override;
		void RenderDetailPanelContent(size_t hash, const std::string& name) override;

	private:
		static const AssetTabConfig config;
	};

}
