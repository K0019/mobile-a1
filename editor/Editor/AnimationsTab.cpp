#include "Editor/AnimationsTab.h"
#include "Editor/EditorGuiUtils.h"

namespace editor {

	const AssetTabConfig AnimationsTab::config = {
		.name = "Animations",
		.identifier = ICON_FA_PERSON_RUNNING " Animations",
		.icon = ICON_FA_PERSON_RUNNING,
		.payloadType = "ANIMATION_HASH",
		.iconColor = {0.9f, 0.7f, 0.3f, 1.0f},  // Yellow/Orange
		.thumbnailType = ThumbnailCache::AssetType::Texture,  // No animation thumbnails
		.hasThumbnails = false
	};

	const AssetTabConfig& AnimationsTab::GetConfig() const
	{
		return config;
	}

	void AnimationsTab::RenderDetailPanelContent(size_t hash, const std::string& name)
	{
#ifdef IMGUI_ENABLED
		ImGui::Columns(2, nullptr, false);
		ImGui::SetColumnWidth(0, 300);

		// Find meshes with similar folder path (likely compatible)
		ImGui::Text(ICON_FA_CUBE " Related meshes:");
		ImGui::BeginChild("RelatedMeshList", ImVec2(0, 80), true);

		// Extract folder hint from animation name
		std::string animFolder;
		// Try to find a common prefix pattern
		// e.g., "mc_animatedIdle" -> look for "mc_animated" meshes
		for (size_t i = 3; i < name.length(); ++i)
		{
			if (std::isupper(name[i]) || name[i] == '_')
			{
				animFolder = name.substr(0, i);
				break;
			}
		}

		int relatedCount = 0;
		if (!animFolder.empty())
		{
			for (const auto& [meshHash, meshRef] : ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceMesh>().Editor_GetAllResources())
			{
				const std::string* meshName = ST<AssetManager>::Get()->Editor_GetName(meshHash);
				if (meshName && meshName->find(animFolder) != std::string::npos)
				{
					ImGui::Selectable(meshName->c_str(), false);
					if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
					{
						size_t dragHash = meshHash.get();
						ImGui::SetDragDropPayload("MESH_HASH", &dragHash, sizeof(size_t));
						ImGui::Text("Mesh: %s", meshName->c_str());
						ImGui::EndDragDropSource();
					}
					relatedCount++;
				}
			}
		}

		if (relatedCount == 0)
		{
			ImGui::TextDisabled("No related meshes found");
		}

		ImGui::EndChild();

		ImGui::NextColumn();

		// Right column - Actions
		ImGui::Text("Actions:");
		ImGui::Spacing();

		ImGui::Columns(1);
#endif
	}

}
