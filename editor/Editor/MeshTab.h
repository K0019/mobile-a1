#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class MeshTab : public GenericAssetTab<ResourceMesh>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderDetailPanelContent(size_t hash, const std::string& name) override;
		bool ShouldShowDetailPanel(size_t hash) override;

	private:
		static const AssetTabConfig config;
	};

}
