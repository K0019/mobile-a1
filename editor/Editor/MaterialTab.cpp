#include "Editor/MaterialTab.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/MaterialCreation.h"

namespace editor {

	const AssetTabConfig MaterialTab::config = {
		.name = "Materials",
		.identifier = ICON_FA_PALETTE " Materials",
		.icon = ICON_FA_PALETTE,
		.payloadType = "MATERIAL_HASH",
		.iconColor = {1.0f, 0.6f, 0.2f, 1.0f},  // Orange
		.thumbnailType = ThumbnailCache::AssetType::Material,
		.hasThumbnails = false  // Material thumbnails not yet implemented
	};

	const AssetTabConfig& MaterialTab::GetConfig() const
	{
		return config;
	}

	void MaterialTab::RenderContextMenuItems(size_t hash)
	{
#ifdef IMGUI_ENABLED
		if (ImGui::MenuItem(ICON_FA_FILE_IMPORT " Edit Material"))
		{
			editor::CreateGuiWindow<editor::MaterialEditWindow>(hash);
		}
#endif
	}

	void MaterialTab::RenderDetailPanelContent(size_t hash, const std::string& name)
	{
#ifdef IMGUI_ENABLED
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 300);

		// Left column - Meshes using this material
		ImGui::Text(ICON_FA_CUBE " Used by meshes:");
		ImGui::BeginChild("MeshUsageList", ImVec2(0, 80), true);

		int usageCount = 0;
		for (const auto& [meshHash, meshRef] : ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
		{
			// Check if this mesh uses the selected material
			const ResourceMesh& mesh = meshRef.get();
			bool usesMaterial = false;
			for (size_t matHash : mesh.defaultMaterialHashes)
			{
				if (matHash == hash)
				{
					usesMaterial = true;
					break;
				}
			}

			if (usesMaterial)
			{
				const std::string* meshName = ST<AssetManager>::Get()->Editor_GetName(meshHash);
				if (meshName)
				{
					ImGui::Selectable(meshName->c_str(), false);

					// Drag source for mesh
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						size_t dragHash = meshHash.get();
						ImGui::SetDragDropPayload("MESH_HASH", &dragHash, sizeof(size_t));
						ImGui::Text("Mesh: %s", meshName->c_str());
						ImGui::EndDragDropSource();
					}
				}
				usageCount++;
			}
		}

		if (usageCount == 0)
		{
			ImGui::TextDisabled("Not used by any mesh");
		}

		ImGui::EndChild();

		ImGui::NextColumn();

		// Right column - Actions
		ImGui::Text("Actions:");
		ImGui::Spacing();

		if (ImGui::Button(ICON_FA_PEN " Edit Material", ImVec2(-1, 0)))
		{
			editor::CreateGuiWindow<editor::MaterialEditWindow>(hash);
		}

		ImGui::Columns(1);
#endif
	}

}
