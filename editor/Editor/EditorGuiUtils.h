#pragma once
#include "Editor/Containers/GUICollection.h"

namespace gui
{
	struct GridHelper
	{
		int itemCount;
		int columnsCount;

		GridHelper(float gridWidth, float thumbnailSize);
		void NextItem();
	};

	class GridItem
	{
	public:
		GridItem(int id, bool doSameLine, float thumbnailSize, const std::string* name);
		GridItem(const GridItem&) = delete;
		~GridItem();

	private:
		// So Group dies before doing SameLine(), do a bit of hacking with memory so we control the lifetime ourselves
		struct PerItemVars
		{
			gui::SetID id;
			gui::Group group;
		};
		std::array<std::uint8_t, sizeof(PerItemVars)> varsMem;
		bool doSameLine;
		float thumbnailSize;
		const std::string* name;
	};

	class NewGridHelper
	{
	private:
		float thumbnailSize;
		int itemCount;
		int columnsCount;
		std::string itemName;

		gui::SetStyleVar itemSpacing;
		gui::SetStyleVar framePadding;

	public:
		NewGridHelper(float thumbnailSize);

		// Simply keeps elements at the same y level until it should wrap to the next row
		[[nodiscard]] inline GridItem Item()
		{
			++itemCount;
			return GridItem{ itemCount, itemCount % columnsCount != 0, thumbnailSize, nullptr };
		}
		// In addition to above, adds a rendering of a text at the bottom and while hovering
		[[nodiscard]] inline GridItem Item(const std::string& name)
		{
			itemName = name;
			++itemCount;
			return GridItem{ itemCount, itemCount % columnsCount != 0, thumbnailSize, &itemName };
		}

		int GetItemCount() const;

	};

	std::string TruncateText(std::string text, float maxWidth);
	void ThumbnailLabel(const std::string& text, float thumbnailWidth);

	void ShowSimpleHoverTooltip(const std::string& text);


	template<typename DataType>
	void PayloadSourceWithImageTooltip(const char* identifier, const DataType& data, gui::TextureID textureHandle, gui::Vec2 size)
	{
		if (gui::PayloadSource payload{ identifier, data })
			gui::Image(textureHandle, size);
	}
}