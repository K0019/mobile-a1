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
		GridItem(int id, bool doSameLine);
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
	};

	class NewGridHelper
	{
	private:
		int itemCount;
		int columnsCount;

		gui::SetStyleVar itemSpacing;
		gui::SetStyleVar framePadding;

	public:
		NewGridHelper(float thumbnailSize);

		[[nodiscard]] inline GridItem Item()
		{
			++itemCount;
			return GridItem{ itemCount, itemCount % columnsCount != 0 };
		}

	};

	std::string TruncateText(const std::string& text, float maxWidth);
	void ThumbnailLabel(const std::string& text, float thumbnailWidth);

	void ShowSimpleHoverTooltip(const std::string& text);


	template<typename DataType>
	void PayloadSourceWithImageTooltip(const char* identifier, const DataType& data, gui::TextureID textureHandle, gui::Vec2 size)
	{
		if (gui::PayloadSource payload{ identifier, data })
			gui::Image(textureHandle, size);
	}
}