#pragma once
#include "GUIAsECS.h"
#include "processed_assets.h"

namespace editor {

#define MATERIAL_TEXTURE_INDEX_ENUM \
X(BASE_COLOR_TEXTURE, "Base Color") \
X(METALLIC_ROUGHNESS_TEXTURE, "Metallic Roughness") \
X(NORMAL_TEXTURE, "Normal Map") \
X(EMISSIVE_TEXTURE, "Emissive") \
X(OCCLUSION_TEXTURE, "Occlusion")

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
		char materialName[128]{ "NewMaterial" };

		std::array<size_t, +MATERIAL_TEXTURE_INDEX::TOTAL> textures;
		AssetLoading::ProcessedMaterial materialProps;
	};

}
