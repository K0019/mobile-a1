#include "Editor/MeshTab.h"
#include "Editor/EditorGuiUtils.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"

namespace editor {

	const AssetTabConfig MeshTab::config = {
		.name = "Meshes",
		.identifier = ICON_FA_CUBE " Meshes",
		.icon = ICON_FA_CUBE,
		.payloadType = "MESH_HASH",
		.iconColor = {0.5f, 0.9f, 0.5f, 1.0f},  // Green
		.thumbnailType = ThumbnailCache::AssetType::Mesh,
		.hasThumbnails = false  // Mesh thumbnails not yet implemented
	};

	const AssetTabConfig& MeshTab::GetConfig() const
	{
		return config;
	}

	bool MeshTab::ShouldShowDetailPanel(size_t hash)
	{
#ifdef IMGUI_ENABLED
		auto& container = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>();
		ResourceMesh* mesh = const_cast<ResourceMesh*>(static_cast<const ResourceMesh*>(container.GetResource(hash)));

		if (!mesh || !mesh->IsLoaded())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.4f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Not loaded");
			ImGui::TextDisabled("This mesh is registered but not loaded into memory.");
			ImGui::TextDisabled("Drag to scene or use in a component to load it.");
			ImGui::Spacing();
			return false;
		}
		return true;
#else
		return false;
#endif
	}

	void MeshTab::RenderDetailPanelContent(size_t hash, [[maybe_unused]] const std::string& name)
	{
#ifdef IMGUI_ENABLED
		auto& container = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>();
		ResourceMesh* mesh = const_cast<ResourceMesh*>(static_cast<const ResourceMesh*>(container.GetResource(hash)));

		if (!mesh)
			return;

		auto& resourceManager = ST<GraphicsMain>::Get()->GetAssetSystem();

		// Mesh statistics
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Submeshes:");
		ImGui::SameLine(100);
		ImGui::Text("%zu", mesh->handles.size());

		// Aggregate vertex/index counts across all submeshes
		uint32_t totalVertices = 0;
		uint32_t totalIndices = 0;
		bool hasSkinning = false;
		bool hasMorphs = false;

		for (const auto& handle : mesh->handles)
		{
			if (const auto* meshData = resourceManager.getMesh(handle))
			{
				totalVertices += meshData->vertexCount;
				totalIndices += meshData->indexCount;
				if (meshData->animation.jointCount > 0)
					hasSkinning = true;
				if (meshData->animation.morphTargetCount > 0)
					hasMorphs = true;
			}
		}

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Vertices:");
		ImGui::SameLine(100);
		ImGui::Text("%s", (totalVertices > 1000) ? (std::to_string(totalVertices / 1000) + "K").c_str() : std::to_string(totalVertices).c_str());

		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Triangles:");
		ImGui::SameLine(100);
		uint32_t triangles = totalIndices / 3;
		ImGui::Text("%s", (triangles > 1000) ? (std::to_string(triangles / 1000) + "K").c_str() : std::to_string(triangles).c_str());

		// Features
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Features:");
		ImGui::SameLine(100);
		if (hasSkinning || hasMorphs)
		{
			if (hasSkinning)
				ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), ICON_FA_BONE " Skinned");
			if (hasSkinning && hasMorphs)
				ImGui::SameLine();
			if (hasMorphs)
				ImGui::TextColored(ImVec4(0.8f, 0.6f, 1.0f, 1.0f), ICON_FA_FACE_SMILE " Morphs");
		}
		else
		{
			ImGui::TextDisabled("Static");
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Materials section
		if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_DefaultOpen))
		{
			for (size_t i = 0; i < mesh->defaultMaterialHashes.size(); ++i)
			{
				size_t matHash = mesh->defaultMaterialHashes[i];
				const std::string* matName = ST<AssetManager>::Get()->Editor_GetName(matHash);

				ImGui::PushID(static_cast<int>(i));
				if (matName)
				{
					// Truncate long material names for display
					std::string displayName = *matName;
					if (displayName.length() > 25)
						displayName = displayName.substr(0, 22) + "...";

					ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), ICON_FA_PALETTE);
					ImGui::SameLine();
					ImGui::Selectable(displayName.c_str(), false);

					// Drag source for material
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						ImGui::SetDragDropPayload("MATERIAL_HASH", &matHash, sizeof(size_t));
						ImGui::Text("Material: %s", matName->c_str());
						ImGui::EndDragDropSource();
					}

					// Tooltip with full name
					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("%s", matName->c_str());
						ImGui::EndTooltip();
					}
				}
				else
				{
					ImGui::TextDisabled("[Hash: %zu]", matHash);
				}
				ImGui::PopID();
			}
			ImGui::TreePop();
		}
#endif
	}

}
