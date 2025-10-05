#include "MaterialCreation.h"
#include "MaterialSerialization.h"
#include "ResourceManager.h"
#include "Popup.h"
#include "asset_types.h"
#include "GameSettings.h"
#include "ResourceImporter.h"

namespace editor {

#define X(type, str) str,
	std::array<const char*, +MATERIAL_TEXTURE_INDEX::TOTAL> textureSlotNames{
		MATERIAL_TEXTURE_INDEX_ENUM
	};
#undef X

	MaterialCreationWindow::MaterialCreationWindow()
		: WindowBase{ "Create Material", gui::Vec2{ 400, 200 } }
	{
		textures.fill(0);	//use 0 to represent no texture
	}

	void MaterialCreationWindow::DrawWindow()
	{
		// Name
		gui::TextBox("Material Name", materialName, sizeof(materialName));
		gui::Separator();

		ImGui::ColorEdit4("Base Color", &materialProps.baseColorFactor.x);
		ImGui::SliderFloat("Metallic", &materialProps.metallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &materialProps.roughnessFactor, 0.0f, 1.0f);
		ImGui::ColorEdit3("Emissive Factor", &materialProps.emissiveFactor.x);

		gui::Separator();

		ImGui::SliderFloat("Normal Scale", &materialProps.normalScale, 0.0f, 2.0f);
		ImGui::SliderFloat("Occlusion Strength", &materialProps.occlusionStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Alpha Cutoff", &materialProps.alphaCutoff, 0.0f, 1.0f);

		const char* alphaModes[] = { "Opaque", "Mask", "Blend" };
		int currentAlphaMode = static_cast<int>(materialProps.alphaMode);
		if (ImGui::Combo("Alpha Mode", &currentAlphaMode, alphaModes, 3))
		{
			materialProps.alphaMode = static_cast<AlphaMode>(currentAlphaMode);
		}

		gui::Separator();

		for (int i{}; i < +MATERIAL_TEXTURE_INDEX::TOTAL; ++i)
		{
			gui::SetID id{ i };

			gui::TextBoxReadOnly(textureSlotNames[i], std::to_string(textures[i]));
			gui::PayloadTarget<size_t>("TEXTURE_HASH", [this, i](size_t hash) -> void {
				textures[i] = hash;
			});
			
			gui::SameLine();

			if (gui::Button{ ICON_FA_XMARK })
			{
				textures[i] = 0; // clear texture slot
			}
		}

		if (gui::Button createButton{ "Create" })
			AttemptCreateMaterial();
	}

	void MaterialCreationWindow::AttemptCreateMaterial()
	{
		bool valid = false;
		auto texturesGetter{ ResourceManager::Textures() };
		for (size_t resourceHash : textures)
		{
			if (resourceHash == 0) continue;

			if (!texturesGetter.GetResource(resourceHash))
			{
				ST<Popup>::Get()->Open("Failed to create material", "Invalid texture");
				return;
			}
			valid = true;
		}

		if (valid)
		{
			std::filesystem::path assetDir = ST<Filepaths>::Get()->assets + "/Materials/";
			std::filesystem::create_directories(assetDir);
			std::filesystem::path assetPath = assetDir / (std::string(materialName) + ".material");

			Serializer writer(assetPath.string());
			if (!writer.IsOpen())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not open file for writing: " + assetPath.string());
				return;
			}

			materialProps.name = materialName;
			MaterialSerialization::Serialize(writer, materialProps, textures);

			if (!writer.SaveAndClose())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not save file. Asset not imported: " + assetPath.string());
				return;
			}

			ST<Popup>::Get()->Open("Success!", "Material created at: " + assetPath.string());
			CONSOLE_LOG(LEVEL_INFO) << "Material asset created: " << assetPath;

			ResourceImporter::Import(assetPath);
		}
		else
		{
			ST<Popup>::Get()->Open("Failed to create material", "No textures assigned");
		}


	}

}
