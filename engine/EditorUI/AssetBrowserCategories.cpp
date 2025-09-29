#include "AssetBrowserCategories.h"
#include "AssetBrowser.h"
#include "GUICollection.h"

namespace editor {

    bool MatchesFilter([[maybe_unused]] const std::string& name)
    {
#ifdef IMGUI_ENABLED
        std::string lowerName = name;
        std::string lowerFilter = ST<AssetBrowser>::Get()->searchBuffer;
        std::ranges::transform(lowerName, lowerName.begin(), util::ToLower);
        std::ranges::transform(lowerFilter, lowerFilter.begin(), util::ToLower);
        return lowerName.find(lowerFilter) != std::string::npos;
#else
        return false;
#endif
    }

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

}
