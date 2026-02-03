#include "Editor/EditorGuiUtils.h"
#include "Engine/Resources/ResourceManager.h"

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

	GridItem::GridItem(int id, bool doSameLine, float thumbnailSize, const std::string* name)
		: varsMem{ 0 }
		, doSameLine{ doSameLine }
		, thumbnailSize{ thumbnailSize }
		, name{ name }
	{
		// Initialize id and group
		new (&varsMem[0]) PerItemVars{ id };
	}
	GridItem::~GridItem()
	{
		if (name)
		{
			gui::ShowSimpleHoverTooltip(*name);
			gui::ThumbnailLabel(*name, thumbnailSize);
		}

		// Free id and group
		reinterpret_cast<PerItemVars*>(&varsMem[0])->~PerItemVars();

		if (doSameLine)
			gui::SameLine();
	}

	NewGridHelper::NewGridHelper(float thumbnailSize)
		: thumbnailSize{ thumbnailSize }
		, itemCount{}
		, columnsCount{ std::max(static_cast<int>(gui::GetAvailableContentRegion().x / thumbnailSize), 1) }
		, itemSpacing{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 5.0f, 5.0f } }
		, framePadding{ gui::FLAG_STYLE_VAR::FRAME_PADDING, gui::Vec2{ 2.0f, 2.0f } }
	{
	}

	std::string TruncateText(std::string text, float maxWidth)
	{
		float textSize = gui::CalcTextSize(text).x;
		while (textSize > maxWidth && text.length() > 3)
		{
			text = text.substr(0, text.length() - 4) + "...";
			textSize = gui::CalcTextSize(text).x;
		}
		return text;
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
