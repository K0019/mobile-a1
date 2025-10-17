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