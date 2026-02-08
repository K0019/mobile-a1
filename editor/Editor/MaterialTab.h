#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class MaterialTab : public GenericResourceAssetTab<ResourceMaterial>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderContextMenuItems(const size_t& hash) override;
		void RenderDetailPanelContent(const size_t& hash, const std::string& name) override;

	private:
		static const AssetTabConfig config;
	};

}
