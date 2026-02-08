#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypes.h"

namespace editor {

	class TextureTab : public GenericResourceAssetTab<ResourceTexture>
	{
	protected:
		const AssetTabConfig& GetConfig() const override;
		void RenderDetailPanelContent(const size_t& hash, const std::string& name) override;

	private:
		static const AssetTabConfig config;
	};

}
