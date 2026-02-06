#include "Editor/AssetBrowserCategories.h"
#include "Editor/AssetBrowser.h"
#include "Editor/Containers/GUICollection.h"

namespace editor {

    void BaseAssetCategory::RenderBreadcrumb()
    {
#ifdef IMGUI_ENABLED
        float windowWidth = ImGui::GetContentRegionAvail().x;
        float searchWidth = 300;
        float spacing = ImGui::GetStyle().ItemSpacing.x;

        std::string location = "Imported > " + std::string{ GetName() };

        // Calculate maximum width for location text
        float maxLocationWidth = windowWidth - searchWidth - spacing * 2;
        ImVec2 textSize = ImGui::CalcTextSize(location.c_str());

        if (textSize.x > maxLocationWidth)
        {
            // Truncate text if needed
            while (textSize.x > maxLocationWidth && location.length() > 3)
            {
                location = location.substr(0, location.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(location.c_str());
            }
        }

        ImGui::Text("%s", location.c_str());
#endif
    }

    void GenericStringAssetTab::ForEachAssetItem(const gui::TextBoxWithFilter& filter, std::function<void(const std::string&, const std::string&)> itemCallback)
    {
        for (const std::string& item : GetItemList())
        {
            std::string displayName = GetDisplayName(item);
            if (!filter.PassFilter(displayName))
                continue;

            itemCallback(item, displayName);
        }
    }

    bool GenericStringAssetTab::IsAssetValid([[maybe_unused]] const std::string& asset) const
    {
        return true;
    }

    bool GenericStringAssetTab::RenderGridItemButton(const std::string& asset, const std::string& assetName, const gui::Vec4& iconColor)
    {
        gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, iconColor };
        return gui::Button{ GetConfig().icon, gui::Vec2{ AssetBrowserSettings::thumbnailSize, AssetBrowserSettings::thumbnailSize } };
    }

    std::string GenericStringAssetTab::GetDisplayName(const std::string& item) const
    {
        return item;
    }

}
