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


	std::string TruncateText(const std::string& text, float maxWidth)
	{
		std::string displayName = text;
		ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
		if (textSize.x > maxWidth)
		{
			while (textSize.x > maxWidth && displayName.length() > 3)
			{
				displayName = displayName.substr(0, displayName.length() - 4) + "...";
				textSize = ImGui::CalcTextSize(displayName.c_str());
			}
		}

		return displayName;
	}

	void ThumbnailLabel(const std::string& text, float thumbnailWidth)
	{
		std::string displayName = TruncateText(text, thumbnailWidth);
		ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
		float textX = (thumbnailWidth - textSize.x) * 0.5f;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
		ImGui::TextUnformatted(displayName.c_str());
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

	template<>
	void PayloadSourceWithImageTooltip<std::string>(const char* identifier, const std::string& data, VkDescriptorSet textureHandle, ImVec2 size)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(identifier, data.c_str(), data.size() + 1);

			ImGui::Image(0, size);

			ImGui::EndDragDropSource();
		}
	}
}
