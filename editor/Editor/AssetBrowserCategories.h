#pragma once
#include "Managers/Filesystem.h"
#include "Editor/Containers/GUICollection.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/AssetBrowserSettings.h"
#include "Editor/ThumbnailCache.h"
#include "Assets/AssetManager.h"
#include <optional>

namespace editor {

	// Declares which standard toolbar controls a tab wants displayed
	struct ToolbarCapabilities
	{
		bool showViewToggle = true;      // Grid/List view toggle buttons
		bool showThumbnailSlider = true; // Thumbnail size slider (grid mode only)
		bool showGlobalSearch = true;    // Global search filter
	};

	struct BaseAssetCategory
	{
		virtual ~BaseAssetCategory() = default;
		virtual const char* GetName() const = 0;
		virtual const char* GetIdentifier() const = 0;
		virtual void RenderBreadcrumb();
		virtual void Render(const gui::TextBoxWithFilter& filter) = 0;

		// Toolbar configuration - override to customize which standard controls are shown
		virtual ToolbarCapabilities GetToolbarCapabilities() const { return {}; }

		// Hook for tabs to add custom controls to the toolbar (called after breadcrumb)
		virtual void RenderToolbarExtras() {}
	};

	// Configuration for asset tabs
	struct AssetTabConfig
	{
		const char* name;                              // Display name (e.g., "Textures")
		const char* identifier;                        // Sidebar identifier with icon
		const char* icon;                              // Icon for display
		const char* payloadType;                       // Drag-drop payload type (nullptr = no drag)
		gui::Vec4 iconColor;                           // Color for the icon
		ThumbnailCache::AssetType thumbnailType;       // Thumbnail cache type
		bool hasThumbnails = false;                    // Whether to try loading thumbnails
		float detailPanelWidth = 220.0f;               // Width of detail panel when visible (side panel)
		bool useStringPayload = false;                 // Use string payload instead of size_t hash
	};

	// Unified asset tab template for AssetManager-based resources
	template<typename ResourceType>
	class GenericAssetTab : public BaseAssetCategory
	{
	public:
		const char* GetName() const override { return GetConfig().name; }
		const char* GetIdentifier() const override { return GetConfig().identifier; }

		void Render(const gui::TextBoxWithFilter& filter) override
		{
#ifdef IMGUI_ENABLED
			RenderToolbar();

			const auto& config = GetConfig();
			bool hasSelection = selectedHash.has_value();
			float detailPanelWidth = hasSelection ? config.detailPanelWidth : 0.0f;
			float availableWidth = ImGui::GetContentRegionAvail().x;

			// Asset grid/list (left side)
			RenderAssetView(filter, availableWidth - detailPanelWidth);

			// Detail panel (right side)
			if (hasSelection)
			{
				ImGui::SameLine();
				ImGui::BeginChild("AssetDetails", ImVec2(0, 0), ImGuiChildFlags_Borders);
				RenderDetailPanel();
				ImGui::EndChild();
			}
#endif
		}

	protected:
		virtual const AssetTabConfig& GetConfig() const = 0;
		virtual void RenderToolbar() {}
		virtual void RenderContextMenuItems([[maybe_unused]] size_t hash) {}
		virtual void RenderDetailPanelContent([[maybe_unused]] size_t hash, [[maybe_unused]] const std::string& name) {}
		virtual bool ShouldShowDetailPanel([[maybe_unused]] size_t hash) { return true; }

		std::optional<size_t>& GetSelection() { return selectedHash; }
		const std::optional<size_t>& GetSelection() const { return selectedHash; }

	private:
		std::optional<size_t> selectedHash;

		void RenderAssetView(const gui::TextBoxWithFilter& filter, float width)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();
			const bool listMode = (AssetBrowserSettings::viewMode == AssetBrowserSettings::ViewMode::List);
			float THUMBNAIL_SIZE = AssetBrowserSettings::thumbnailSize;

			ImGui::BeginChild("AssetGrid", ImVec2(width, 0), false);

			float panelWidth = ImGui::GetContentRegionAvail().x;
			gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

			ImVec4 iconColor(config.iconColor.x, config.iconColor.y, config.iconColor.z, config.iconColor.w);

			int count = 0;
			for (const auto& [hash, resourceRef] : ST<AssetManager>::Get()->Editor_GetContainer<ResourceType>().Editor_GetAllResources())
			{
				const std::string& assetName = *ST<AssetManager>::Get()->Editor_GetName(hash);
				if (!filter.PassFilter(assetName))
					continue;

				bool isSelected = selectedHash.has_value() && selectedHash.value() == hash.get();
				gui::SetID id{ count };

				if (listMode)
				{
					RenderListItem(hash.get(), assetName, isSelected, count, iconColor);
				}
				else
				{
					RenderGridItem(hash.get(), assetName, isSelected, count, THUMBNAIL_SIZE, iconColor);
					grid.NextItem();
				}
				count++;
			}

			ImGui::PopStyleVar(2);
			ImGui::EndChild();
#endif
		}

		void RenderListItem(size_t hash, const std::string& name, bool isSelected, int index, const ImVec4& iconColor)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();

			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.3f));
			if (ImGui::Selectable(("##sel" + std::to_string(index)).c_str(), isSelected, 0, ImVec2(0, 20)))
			{
				selectedHash = isSelected ? std::nullopt : std::optional<size_t>(hash);
			}
			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem(("ctx" + std::to_string(index)).c_str()))
			{
				RenderContextMenuItems(hash);
				ImGui::EndPopup();
			}

			if (config.payloadType)
				gui::PayloadSource{ config.payloadType, hash };

			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosX(8);
			ImGui::TextColored(iconColor, "%s", config.icon);
			ImGui::SameLine();
			ImGui::Text("%s", name.c_str());
#endif
		}

		void RenderGridItem(size_t hash, const std::string& name, bool isSelected, int index, float thumbSize, const ImVec4& iconColor)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();
			gui::Group group;

			if (isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));

			bool clicked = false;
			uint64_t thumbId = 0;
			if (config.hasThumbnails)
				thumbId = ThumbnailCache::Get().GetThumbnail(hash, config.thumbnailType, name);

			if (thumbId != 0)
			{
				clicked = ImGui::ImageButton(
					("##item" + std::to_string(index)).c_str(),
					static_cast<ImTextureID>(thumbId),
					ImVec2(thumbSize, thumbSize),
					ImVec2(0, 1), ImVec2(1, 0));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
				clicked = ImGui::Button(
					(std::string(config.icon) + "##" + std::to_string(index)).c_str(),
					ImVec2(thumbSize, thumbSize));
				ImGui::PopStyleColor();
			}

			if (clicked)
				selectedHash = isSelected ? std::nullopt : std::optional<size_t>(hash);

			if (isSelected)
				ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem(("ctx" + std::to_string(index)).c_str()))
			{
				RenderContextMenuItems(hash);
				ImGui::EndPopup();
			}

			if (config.payloadType)
				gui::PayloadSource{ config.payloadType, hash };

			gui::ShowSimpleHoverTooltip(name);
			gui::ThumbnailLabel(name, thumbSize);
#endif
		}

		void RenderDetailPanel()
		{
#ifdef IMGUI_ENABLED
			if (!selectedHash.has_value())
				return;

			const auto& config = GetConfig();
			size_t hash = selectedHash.value();
			const std::string* name = ST<AssetManager>::Get()->Editor_GetName(hash);

			if (!name)
			{
				ImGui::TextColored(ImVec4(1, 0.4f, 0.4f, 1), "%s not found", config.name);
				return;
			}

			ImGui::TextColored(ImVec4(config.iconColor.x, config.iconColor.y, config.iconColor.z, config.iconColor.w), "%s", config.icon);
			ImGui::SameLine();
			ImGui::TextWrapped("%s", name->c_str());
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);
			if (ImGui::SmallButton(ICON_FA_XMARK))
			{
				selectedHash.reset();
				return;
			}

			ImGui::Separator();

			if (ShouldShowDetailPanel(hash))
				RenderDetailPanelContent(hash, *name);

			if (config.payloadType)
			{
				ImGui::Button(ICON_FA_HAND " Drag to assign", ImVec2(-1, 0));
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::SetDragDropPayload(config.payloadType, &hash, sizeof(size_t));
					ImGui::Text("%s: %s", config.name, name->c_str());
					ImGui::EndDragDropSource();
				}
			}
#endif
		}
	};

	// Asset tab for string-based items (prefabs, scripts, scenes, etc.)
	class StringAssetTab : public BaseAssetCategory
	{
	public:
		const char* GetName() const override { return GetConfig().name; }
		const char* GetIdentifier() const override { return GetConfig().identifier; }

		void Render(const gui::TextBoxWithFilter& filter) override
		{
#ifdef IMGUI_ENABLED
			RenderToolbar();

			const auto& config = GetConfig();
			bool hasSelection = selectedItem.has_value() && HasDetailPanel();
			float detailPanelWidth = hasSelection ? config.detailPanelWidth : 0.0f;
			float availableWidth = ImGui::GetContentRegionAvail().x;

			// Asset grid/list (left side)
			RenderAssetView(filter, availableWidth - detailPanelWidth);

			// Detail panel (right side)
			if (hasSelection)
			{
				ImGui::SameLine();
				ImGui::BeginChild("AssetDetails", ImVec2(0, 0), ImGuiChildFlags_Borders);
				RenderDetailPanel();
				ImGui::EndChild();
			}
#endif
		}

	protected:
		virtual const AssetTabConfig& GetConfig() const = 0;
		virtual std::vector<std::string> GetItemList() const = 0;
		virtual std::string GetDisplayName(const std::string& item) const { return item; }
		virtual void RenderToolbar() {}
		virtual void OnItemClicked([[maybe_unused]] const std::string& item) {}
		virtual void RenderContextMenuItems([[maybe_unused]] const std::string& item) {}
		virtual void RenderDetailPanelContent([[maybe_unused]] const std::string& item) {}
		virtual bool HasDetailPanel() const { return false; }

		std::optional<std::string>& GetSelection() { return selectedItem; }

	private:
		std::optional<std::string> selectedItem;

		void RenderAssetView(const gui::TextBoxWithFilter& filter, float width)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();
			const bool listMode = (AssetBrowserSettings::viewMode == AssetBrowserSettings::ViewMode::List);
			float THUMBNAIL_SIZE = AssetBrowserSettings::thumbnailSize;

			ImGui::BeginChild("AssetGrid", ImVec2(width, 0), false);

			float panelWidth = ImGui::GetContentRegionAvail().x;
			gui::GridHelper grid(panelWidth, THUMBNAIL_SIZE + 10);

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

			ImVec4 iconColor(config.iconColor.x, config.iconColor.y, config.iconColor.z, config.iconColor.w);

			int count = 0;
			for (const std::string& item : GetItemList())
			{
				std::string displayName = GetDisplayName(item);
				if (!filter.PassFilter(displayName))
					continue;

				bool isSelected = selectedItem.has_value() && selectedItem.value() == item;
				gui::SetID id{ count };

				if (listMode)
				{
					RenderListItem(item, displayName, isSelected, count, iconColor);
				}
				else
				{
					RenderGridItem(item, displayName, isSelected, count, THUMBNAIL_SIZE, iconColor);
					grid.NextItem();
				}
				count++;
			}

			ImGui::PopStyleVar(2);
			ImGui::EndChild();
#endif
		}

		void RenderListItem(const std::string& item, const std::string& displayName, bool isSelected, int index, const ImVec4& iconColor)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();

			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.26f, 0.59f, 0.98f, 0.3f));
			if (ImGui::Selectable(("##sel" + std::to_string(index)).c_str(), isSelected, 0, ImVec2(0, 20)))
			{
				if (isSelected)
					selectedItem.reset();
				else
				{
					selectedItem = item;
					OnItemClicked(item);
				}
			}
			ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem(("ctx" + std::to_string(index)).c_str()))
			{
				RenderContextMenuItems(item);
				ImGui::EndPopup();
			}

			if (config.payloadType)
				gui::PayloadSource<std::string>{ config.payloadType, item.c_str(), (std::string(config.icon) + " " + displayName).c_str() };

			ImGui::SameLine(0, 0);
			ImGui::SetCursorPosX(8);
			ImGui::TextColored(iconColor, "%s", config.icon);
			ImGui::SameLine();
			ImGui::Text("%s", displayName.c_str());
#endif
		}

		void RenderGridItem(const std::string& item, const std::string& displayName, bool isSelected, int index, float thumbSize, const ImVec4& iconColor)
		{
#ifdef IMGUI_ENABLED
			const auto& config = GetConfig();
			gui::Group group;

			if (isSelected)
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));

			ImGui::PushStyleColor(ImGuiCol_Text, iconColor);
			bool clicked = ImGui::Button(
				(std::string(config.icon) + "##" + std::to_string(index)).c_str(),
				ImVec2(thumbSize, thumbSize));
			ImGui::PopStyleColor();

			if (clicked)
			{
				if (isSelected)
					selectedItem.reset();
				else
				{
					selectedItem = item;
					OnItemClicked(item);
				}
			}

			if (isSelected)
				ImGui::PopStyleColor();

			if (ImGui::BeginPopupContextItem(("ctx" + std::to_string(index)).c_str()))
			{
				RenderContextMenuItems(item);
				ImGui::EndPopup();
			}

			if (config.payloadType)
				gui::PayloadSource<std::string>{ config.payloadType, item.c_str(), (std::string(config.icon) + " " + displayName).c_str() };

			gui::ShowSimpleHoverTooltip(displayName);
			gui::ThumbnailLabel(displayName, thumbSize);
#endif
		}

		void RenderDetailPanel()
		{
#ifdef IMGUI_ENABLED
			if (!selectedItem.has_value())
				return;

			const auto& config = GetConfig();
			const std::string& item = selectedItem.value();
			std::string displayName = GetDisplayName(item);

			ImGui::TextColored(ImVec4(config.iconColor.x, config.iconColor.y, config.iconColor.z, config.iconColor.w), "%s", config.icon);
			ImGui::SameLine();
			ImGui::TextWrapped("%s", displayName.c_str());
			ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);
			if (ImGui::SmallButton(ICON_FA_XMARK))
			{
				selectedItem.reset();
				return;
			}

			ImGui::Separator();
			RenderDetailPanelContent(item);

			if (config.payloadType)
			{
				ImGui::Button(ICON_FA_HAND " Drag to assign", ImVec2(-1, 0));
				if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
				{
					ImGui::SetDragDropPayload(config.payloadType, item.c_str(), item.size() + 1);
					ImGui::Text("%s: %s", config.name, displayName.c_str());
					ImGui::EndDragDropSource();
				}
			}
#endif
		}
	};

	// Legacy alias for backward compatibility
	using StringListAssetTab = StringAssetTab;

}
