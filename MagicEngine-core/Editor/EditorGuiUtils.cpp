#include "EditorGuiUtils.h"
#include "ResourceManager.h"

namespace gui
{
	GridHelper::GridHelper(float gridWidth, float thumbnailSize)
		: itemCount {0}
	{
		columnsCount = static_cast<int>(gridWidth / thumbnailSize);
		if (columnsCount < 1) columnsCount = 1;
	}

	void GridHelper::NextItem()
	{
		if (++itemCount % columnsCount != 0)
		{
			SameLine();
		}
	}


	std::string TruncateText(const std::string& text, [[maybe_unused]] float maxWidth)
	{
		std::string displayName = text;
#ifdef IMGUI_ENABLED
		ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
		if (textSize.x > maxWidth)
		{
			while (textSize.x > maxWidth && displayName.length() > 3)
			{
				displayName = displayName.substr(0, displayName.length() - 4) + "...";
				textSize = ImGui::CalcTextSize(displayName.c_str());
			}
		}
#endif
		return displayName;
	}

	void ThumbnailLabel([[maybe_unused]] const std::string& text, [[maybe_unused]] float thumbnailWidth)
	{
#ifdef IMGUI_ENABLED
		std::string displayName = TruncateText(text, thumbnailWidth);
		ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
		float textX = (thumbnailWidth - textSize.x) * 0.5f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
		ImGui::TextUnformatted(displayName.c_str());
#endif
	}

	void ShowSimpleHoverTooltip(const std::string& text)
	{
		if (gui::IsItemHovered())
		{
			{
				gui::Tooltip tooltip;
				gui::TextUnformatted(text);
			}
		}
	}

}
