#pragma once
#include "Editor/AssetBrowserCategories.h"
#include <optional>

namespace editor
{
	class AnimationsTab : public editor::BaseAssetCategory
	{
	public:
		const char* GetName() const final;
		const char* GetIdentifier() const final;
		void Render(const gui::TextBoxWithFilter& filter) final;

	private:
		void RenderGridView(const gui::TextBoxWithFilter& filter, float height);
		void RenderDetailPanel();

		std::optional<size_t> selectedAnimationHash;
	};
}