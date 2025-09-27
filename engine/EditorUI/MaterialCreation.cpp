#include "MaterialCreation.h"
#include "ResourceManager.h"
#include "Popup.h"

namespace editor {

#define X(type, str) str,
	std::array<const char*, +MATERIAL_TEXTURE_INDEX::TOTAL> textureSlotNames{
		MATERIAL_TEXTURE_INDEX_ENUM
	};
#undef X

	MaterialCreationWindow::MaterialCreationWindow()
		: WindowBase{ "Create Material", gui::Vec2{ 400, 200 } }
	{
	}

	void MaterialCreationWindow::DrawWindow()
	{
		for (int i{}; i < +MATERIAL_TEXTURE_INDEX::TOTAL; ++i)
		{
			gui::TextBoxReadOnly(textureSlotNames[i], std::to_string(textures[i]));
			gui::PayloadTarget<size_t>("TEXTURE_HASH", [this, i](size_t hash) -> void {
				textures[i] = hash;
			});
		}

		if (gui::Button createButton{ "Create" })
			AttemptCreateMaterial();
	}

	void MaterialCreationWindow::AttemptCreateMaterial()
	{
		auto texturesGetter{ ResourceManager::Textures() };
		for (size_t resourceHash : textures)
			if (!texturesGetter.GetResource(resourceHash))
			{
				ST<Popup>::Get()->Open("Failed to create material", "Invalid texture");
				return;
			}

		CONSOLE_LOG(LEVEL_INFO) << "Textures validation finished. TODO: Create material from textures";
	}

}
