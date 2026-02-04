#include "Editor/MaterialCreation.h"
#include "Assets/MaterialSerialization.h"
#include "Assets/AssetManager.h"
#include "Assets/Types/AssetTypes.h"
#include "Assets/AssetImporter.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"
#include "Editor/EditorUtilResource.h"
#include "resource/resource_types.h"
#include "FilepathConstants.h"
#include "VFS/VFS.h"

namespace editor {

#define X(type, str) str,
	std::array<const char*, +MATERIAL_TEXTURE_INDEX::TOTAL> textureSlotNames{
		MATERIAL_TEXTURE_INDEX_ENUM
	};
#undef X

	MaterialCreationWindow::MaterialCreationWindow()
		: WindowBase{ "Create Material", gui::Vec2{ 400, 500 }, gui::FLAG_WINDOW::NO_RESIZE }
	{
		textures.fill(0);	//use 0 to represent no texture
	}

	void MaterialCreationWindow::DrawWindow()
	{
#ifdef IMGUI_ENABLED
		// ----- Name -----
		gui::TextBox("Material Name", materialName, sizeof(materialName));
		gui::Separator();

		// ----- Core PBR properties -----
		ImGui::ColorEdit4("Base Color", &materialProps.baseColorFactor.x);
		ImGui::SliderFloat("Metallic", &materialProps.metallicFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Roughness", &materialProps.roughnessFactor, 0.0f, 1.0f);
		ImGui::SliderFloat("Emissive", &materialProps.emissiveFactor.x, 0.0f, 10.0f);
		ImGui::SliderFloat("Alpha Cutoff", &materialProps.alphaCutoff, 0.0f, 1.0f);
		gui::Separator();

		// ----- Alpha Mode -----
		const char* alphaModes[] = { "Opaque", "Mask", "Blend" };
		int currentAlphaMode = static_cast<int>(materialProps.alphaMode);
		if (ImGui::Combo("Alpha Mode", &currentAlphaMode, alphaModes, 3))
		{
			materialProps.alphaMode = static_cast<AlphaMode>(currentAlphaMode);
		}
		gui::Separator();

		// ----- Textures -----
		for (int i{}; i < +MATERIAL_TEXTURE_INDEX::TOTAL; ++i)
		{
			gui::SetID id{ i };

			EditorUtil_DrawAssetSlot<ResourceTexture>(textureSlotNames[i], textures[i]);

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
		auto texturesGetter{ AssetManager::GetContainer<ResourceTexture>() };
		for (size_t resourceHash : textures)
		{
			if (resourceHash == 0) continue;

			if (!texturesGetter.GetResource(resourceHash))
			{
				CONSOLE_LOG(LEVEL_ERROR) << "Failed to create material: Invalid texture";
				return;
			}
			valid = true;
		}

		if (!valid)
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Failed to create material: No textures assigned";
			return;
		}

		std::filesystem::path assetDir = "compiledassets/materials/";
		std::filesystem::create_directories(assetDir);
		std::filesystem::path assetPath = assetDir / (std::string(materialName) + ".material");

		Serializer writer(VFS::ConvertVirtualToPhysical(assetPath.string()));
		if (!writer.IsOpen())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to create material: Could not open file for writing: " << assetPath;
			return;
		}

		// Set the flags
		materialProps.flags = 0;
		if (isUnlit)
			materialProps.flags |= MaterialFlags::UNLIT;
		if (isDoubleSided)
			materialProps.flags |= MaterialFlags::DOUBLE_SIDED;
		if (castShadow)
			materialProps.flags |= MaterialFlags::CAST_SHADOW;
		if (receiveShadow)
			materialProps.flags |= MaterialFlags::RECEIVE_SHADOW;

		materialProps.name = materialName;

		MaterialSerialization::Serialize(writer, materialProps, textures);

		if (!writer.SaveAndClose())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to create material: Could not save file: " << assetPath;
			return;
		}

		CONSOLE_LOG(LEVEL_INFO) << "Material created: " << assetPath;
		AssetImporter::Import(assetPath.string());
	}




	MaterialEditWindow::MaterialEditWindow(size_t materialHash)
		:
		WindowBase{ "Edit Material", gui::Vec2{ 400, 500 }, gui::FLAG_WINDOW::NO_RESIZE }
	{
		textures.fill(0);
		LoadMaterial(materialHash);
	}

	void MaterialEditWindow::LoadMaterial(size_t materialHash)
	{
		textures.fill(0);

		// Load the information of the material to be edited
		auto& fpManager = ST<AssetManager>::Get()->INTERNAL_GetFilepathsManager();
		auto materialEntry = fpManager.GetFileEntry(materialHash);

		if (!materialEntry)
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Material entry not found for hash: " << materialHash;
			return;
		}

		materialFilePath = materialEntry->path;
		std::string fileData;

		if (!VFS::ReadFile(materialFilePath, fileData))
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to open .material file: " << materialFilePath;
			return;
		}

		// Build our old material props
		Deserializer reader{ fileData };
		if (!reader.IsValid())
		{
			CONSOLE_LOG(LEVEL_ERROR) << "Failed to deserialize .material file: " << materialFilePath;
			return;
		}
		MaterialSerialization::Deserialize(reader, materialProps);

		// Name
#ifdef GLFW
		strncpy_s(materialName, 128, materialProps.name.c_str(), sizeof(materialName));
#else
		strncpy(materialName, materialProps.name.c_str(), sizeof(materialName));
#endif
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

		// Load flags
		isUnlit = (materialProps.flags & MaterialFlags::UNLIT) != 0;
		isDoubleSided = (materialProps.flags & MaterialFlags::DOUBLE_SIDED) != 0;
		castShadow = (materialProps.flags & MaterialFlags::CAST_SHADOW) != 0;
		receiveShadow = (materialProps.flags & MaterialFlags::RECEIVE_SHADOW) != 0;
	}

	void MaterialEditWindow::DrawWindow()
	{
#ifdef IMGUI_ENABLED
		bool changed = false;

		// ----- Name -----
		changed |= ImGui::InputText("Material Name", materialName, sizeof(materialName));
		gui::Separator();

		// ----- Core PBR properties -----
		changed |= ImGui::ColorEdit4("Base Color", &materialProps.baseColorFactor.x);
		changed |= ImGui::SliderFloat("Metallic", &materialProps.metallicFactor, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Roughness", &materialProps.roughnessFactor, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Emissive", &materialProps.emissiveFactor.x, 0.0f, 10.0f);
		changed |= ImGui::SliderFloat("Alpha Cutoff", &materialProps.alphaCutoff, 0.0f, 1.0f);
		gui::Separator();

		// ----- Alpha Mode -----
		const char* alphaModes[] = { "Opaque", "Mask", "Blend" };
		int currentAlphaMode = static_cast<int>(materialProps.alphaMode);
		if (ImGui::Combo("Alpha Mode", &currentAlphaMode, alphaModes, 3))
		{
			materialProps.alphaMode = static_cast<AlphaMode>(currentAlphaMode);
			changed = true;
		}
		gui::Separator();

		// ----- Textures -----
		for (int i{}; i < +MATERIAL_TEXTURE_INDEX::TOTAL; ++i)
		{
			gui::SetID id{ i };

			size_t prevTexture = textures[i];
			EditorUtil_DrawAssetSlot<ResourceTexture>(textureSlotNames[i], textures[i]);
			changed |= (textures[i] != prevTexture);

			gui::SameLine();

			if (gui::Button{ ICON_FA_XMARK })
			{
				if (textures[i] != 0)
				{
					textures[i] = 0;
					changed = true;
				}
			}
		}

		// Auto-update material when any property changes
		if (changed)
			AttemptUpdateMaterial();
#endif
	}

	void MaterialEditWindow::AttemptUpdateMaterial()
	{
		if (materialFilePath.empty())
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Material update skipped: empty file path";
			return;
		}

		// Validate any assigned textures exist
		auto texturesGetter{ AssetManager::GetContainer<ResourceTexture>() };
		for (size_t resourceHash : textures)
		{
			if (resourceHash == 0) continue;

			if (!texturesGetter.GetResource(resourceHash))
			{
				CONSOLE_LOG(LEVEL_WARNING) << "Material update skipped: invalid texture hash " << resourceHash;
				return;
			}
		}

		// Write to the original file path (update in place)
		std::string physicalPath = VFS::ConvertVirtualToPhysical(materialFilePath);
		Serializer writer(physicalPath);
		if (!writer.IsOpen())
		{
			CONSOLE_LOG(LEVEL_WARNING) << "Material update skipped: could not open " << physicalPath;
			return;
		}

		// Set the flags
		materialProps.flags = 0;
		if (isUnlit)
			materialProps.flags |= MaterialFlags::UNLIT;
		if (isDoubleSided)
			materialProps.flags |= MaterialFlags::DOUBLE_SIDED;
		if (castShadow)
			materialProps.flags |= MaterialFlags::CAST_SHADOW;
		if (receiveShadow)
			materialProps.flags |= MaterialFlags::RECEIVE_SHADOW;

		materialProps.name = materialName;

		MaterialSerialization::Serialize(writer, materialProps, textures);

		if (!writer.SaveAndClose())
			return;

		AssetImporter::Import(materialFilePath);
	}
}
