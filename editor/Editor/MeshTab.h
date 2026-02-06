#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class MeshTab : public GenericResourceAssetTab<ResourceMesh>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderDetailPanelContent(const size_t& hash, const std::string& name) override;
		bool ShouldShowDetailPanel(const size_t& hash) override;

	private:
		static const AssetTabConfig config;
	};

}
