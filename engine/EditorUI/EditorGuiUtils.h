#pragma once
#include "GUICollection.h"


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
	void PayloadSourceWithImageTooltip(const char* identifier, const DataType& data, VkDescriptorSet textureHandle, ImVec2 size)
	{
		if (ImGui::BeginDragDropSource())
		{
			ImGui::SetDragDropPayload(identifier, &data, sizeof(DataType));

			ImGui::Image(0, size);

			ImGui::EndDragDropSource();
		}
	}
	template<>
	void PayloadSourceWithImageTooltip<std::string>(const char* identifier, const std::string& data, VkDescriptorSet textureHandle, ImVec2 size);
}