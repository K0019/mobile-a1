#pragma once
#include "GUIAsECS.h"

namespace editor {

#define MATERIAL_TEXTURE_INDEX_ENUM \
X(NORMAL_COLOR, "Normal Color")

#define X(type, str) type,
	enum class MATERIAL_TEXTURE_INDEX
	{
		MATERIAL_TEXTURE_INDEX_ENUM
		TOTAL
	};
#undef X

	class MaterialCreationWindow : public WindowBase<MaterialCreationWindow, false>
	{
	public:
		MaterialCreationWindow();

		void DrawWindow() override;

	private:
		void AttemptCreateMaterial();

	private:
		std::array<size_t, +MATERIAL_TEXTURE_INDEX::TOTAL> textures;

	};

}
