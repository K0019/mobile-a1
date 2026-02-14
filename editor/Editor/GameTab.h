#pragma once
#include "Editor/AssetBrowserCategories.h"

#define GAMETAB_SUBTAB_TYPE_ENUM \

namespace editor {

	class GameTab : public BaseAssetCategory
	{
	public:
		GameTab();

		const char* GetName() const final;
		const char* GetIdentifier() const final;
		void Render(const gui::TextBoxWithFilter& filter) final;

		void RenderSidebar();

	public:
#define X(type, name) type,
		enum class SUBTAB_TYPE
		{
			GAMETAB_SUBTAB_TYPE_ENUM
			TOTAL
		};
#undef X

	private:
		SUBTAB_TYPE currentSubtab;

		static const std::array<const char*, +SUBTAB_TYPE::TOTAL> SUBTAB_NAMES;
	};

}
GENERATE_ENUM_CLASS_ITERATION_OPERATORS(editor::GameTab::SUBTAB_TYPE)
