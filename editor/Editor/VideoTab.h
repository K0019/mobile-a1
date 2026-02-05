#pragma once
#include "Editor/AssetBrowserCategories.h"
#include "Assets/Types/AssetTypesVideo.h"

namespace editor {

    class VideoTab : public GenericAssetTab<ResourceVideo>
    {
    public:
        const AssetTabConfig& GetConfig() const override;

    protected:
        void RenderDetailPanelContent(size_t hash, const std::string& name) override;

    private:
        static const AssetTabConfig config;
    };

}
