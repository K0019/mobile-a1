#include "Editor/MaterialCreation.h"
#include "Engine/Resources/MaterialSerialization.h"
#include "Engine/Resources/ResourceManager.h"
#include "Engine/Resources/ResourceImporter.h"
#include "Editor/Popup.h"
#include "resource/resource_types.h"
#include "FilepathConstants.h"

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
#ifdef IMGUI_ENABLED
		// ----- Name -----
		gui::TextBox("Material Name", materialName, sizeof(materialName));
		gui::Separator();

		// ----- Shading Model -----
		gui::Checkbox("Unlit", &isUnlit);
		gui::Checkbox("Double Sided", &isDoubleSided);
		gui::Separator();

		// ----- Core PBR properties -----
		ImGui::ColorEdit4("Base Color", &materialProps.baseColorFactor.x);
		ImGui::SliderFloat("Metallic", &materialProps.metallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &materialProps.roughnessFactor, 0.0f, 1.0f);
		ImGui::ColorEdit3("Emissive Factor", &materialProps.emissiveFactor.x);
		ImGui::SliderFloat("Normal Scale", &materialProps.normalScale, 0.0f, 2.0f);
		ImGui::SliderFloat("Occlusion Strength", &materialProps.occlusionStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Alpha Cutoff", &materialProps.alphaCutoff, 0.0f, 1.0f);
		gui::Separator();

		// ----- Material properties -----
		const char* alphaModes[] = { "Opaque", "Mask", "Blend" };
		int currentAlphaMode = static_cast<int>(materialProps.alphaMode);
		if (ImGui::Combo("Alpha Mode", &currentAlphaMode, alphaModes, 3))
		{
			materialProps.alphaMode = static_cast<AlphaMode>(currentAlphaMode);
		}
		gui::Separator();

		// ----- Texturse -----
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
#endif
	}

	void MaterialCreationWindow::AttemptCreateMaterial()
	{
		bool valid = false;
		auto texturesGetter{ MagicResourceManager::Textures() };
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
			std::filesystem::path assetDir = "compiledassets/materials/";
			std::filesystem::create_directories(assetDir);
			std::filesystem::path assetPath = assetDir / (std::string(materialName) + ".material");

			Serializer writer(VFS::ConvertVirtualToPhysical(assetPath.string()));
			if (!writer.IsOpen())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not open file for writing: " + assetPath.string());
				return;
			}

			// Set the flags
			materialProps.flags = 0;
			if (isUnlit)
				materialProps.flags |= MaterialFlags::UNLIT;
			if (isDoubleSided)
				materialProps.flags |= MaterialFlags::DOUBLE_SIDED;

			materialProps.name = materialName;
			
			MaterialSerialization::Serialize(writer, materialProps, textures);

			if (!writer.SaveAndClose())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not save file. Asset not imported: " + assetPath.string());
				return;
			}

			ST<Popup>::Get()->Open("Success!", "Material created at: " + assetPath.string());
			CONSOLE_LOG(LEVEL_INFO) << "Material asset created: " << assetPath;

			ResourceImporter::Import(assetPath.string());
		}
		else
		{
			ST<Popup>::Get()->Open("Failed to create material", "No textures assigned");
		}
	}




	MaterialEditWindow::MaterialEditWindow(size_t materialHash)
		:
		WindowBase{ "Edit Material", gui::Vec2{ 800, 400 } }
	{
		// Load the information of the material to be edited
		auto& fpManager = ST<MagicResourceManager>::Get()->INTERNAL_GetFilepathsManager();
		auto materialEntry = fpManager.GetFileEntry(materialHash);

		assert(materialEntry);

		std::string pathToMaterial = materialEntry->path;
		std::string fileData;

		if (!VFS::ReadFile(pathToMaterial, fileData))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to open .material file ";
		}

		// Build our old material props
		Deserializer reader{ fileData };
		if (!reader.IsValid())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize .material file ";
		}
		MaterialSerialization::Deserialize(reader, materialProps);

		// Name
		strncpy(materialName, materialProps.name.c_str(), sizeof(materialName));
		materialName[sizeof(materialName) - 1] = '\0';

		// Texture hashes
		auto loadTexture = [&](int index, const auto& dataSource)
			{
				if (auto* src = std::get_if<FilePathSource>(&dataSource))
				{
					if (auto entry = fpManager.GetFileEntry(src->path))
						textures[index] = entry->associatedResources[0].hashes[0];
				}
				else
				{
					textures[index] = 0;
				}
			};

		loadTexture(0, materialProps.baseColorTexture.source);
		loadTexture(1, materialProps.metallicRoughnessTexture.source);
		loadTexture(2, materialProps.normalTexture.source);
		loadTexture(3, materialProps.emissiveTexture.source);
		loadTexture(4, materialProps.occlusionTexture.source);
	}

	void MaterialEditWindow::DrawWindow()
	{
#ifdef IMGUI_ENABLED
		// ----- Name -----
		gui::TextBox("Material Name", materialName, sizeof(materialName));
		gui::Separator();

		// ----- Shading Model -----
		gui::Checkbox("Unlit", &isUnlit);
		gui::Checkbox("Double Sided", &isDoubleSided);
		gui::Separator();

		// ----- Core PBR properties -----
		ImGui::ColorEdit4("Base Color", &materialProps.baseColorFactor.x);
		ImGui::SliderFloat("Metallic", &materialProps.metallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &materialProps.roughnessFactor, 0.0f, 1.0f);
		ImGui::ColorEdit3("Emissive Factor", &materialProps.emissiveFactor.x);
		ImGui::SliderFloat("Normal Scale", &materialProps.normalScale, 0.0f, 2.0f);
		ImGui::SliderFloat("Occlusion Strength", &materialProps.occlusionStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Alpha Cutoff", &materialProps.alphaCutoff, 0.0f, 1.0f);
		gui::Separator();

		// ----- Material properties -----
		const char* alphaModes[] = { "Opaque", "Mask", "Blend" };
		int currentAlphaMode = static_cast<int>(materialProps.alphaMode);
		if (ImGui::Combo("Alpha Mode", &currentAlphaMode, alphaModes, 3))
		{
			materialProps.alphaMode = static_cast<AlphaMode>(currentAlphaMode);
		}
		gui::Separator();

		// ----- Texturse -----
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
#endif
	}
	
	void MaterialEditWindow::AttemptCreateMaterial()
	{
		bool valid = false;
		auto texturesGetter{ MagicResourceManager::Textures() };
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
			std::filesystem::path assetDir = "compiledassets/materials/";
			std::filesystem::create_directories(assetDir);
			std::filesystem::path assetPath = assetDir / (std::string(materialName) + ".material");

			Serializer writer(VFS::ConvertVirtualToPhysical(assetPath.string()));
			if (!writer.IsOpen())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not open file for writing: " + assetPath.string());
				return;
			}

			// Set the flags
			materialProps.flags = 0;
			if (isUnlit)
				materialProps.flags |= MaterialFlags::UNLIT;
			if (isDoubleSided)
				materialProps.flags |= MaterialFlags::DOUBLE_SIDED;

			materialProps.name = materialName;

			MaterialSerialization::Serialize(writer, materialProps, textures);

			if (!writer.SaveAndClose())
			{
				ST<Popup>::Get()->Open("Failed to create material", "Could not save file. Asset not imported: " + assetPath.string());
				return;
			}

			ST<Popup>::Get()->Open("Success!", "Material created at: " + assetPath.string());
			CONSOLE_LOG(LEVEL_INFO) << "Material asset created: " << assetPath;

			ResourceImporter::Import(assetPath.string());
		}
		else
		{
			ST<Popup>::Get()->Open("Failed to create material", "No textures assigned");
		}


	}
}
