#pragma once
#include "Managers/Filesystem.h"
#include "Editor/Containers/GUICollection.h"
#include "Editor/EditorGuiUtils.h"
#include "Editor/AssetBrowserSettings.h"
#include "Editor/ThumbnailCache.h"
#include "Assets/AssetManager.h"
#include <optional>
#include <functional>

namespace editor {

	// Declares which standard toolbar controls a tab wants displayed
	struct ToolbarCapabilities
	{
		bool showViewToggle = true;      // Grid/List view toggle buttons
		bool showThumbnailSlider = true; // Thumbnail size slider (grid mode only)
		bool showGlobalSearch = true;    // Global search filter
	};

	class BaseAssetCategory
	{
	public:
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
	};

	// Shared structure for rendering an asset tab
	template <typename AssetIdentificationType> // only size_t (hash) and std::string (name) are implemented
	class GenericAssetTab : public BaseAssetCategory
	{
	public:
		const char* GetName() const override;
		const char* GetIdentifier() const override;

		void Render(const gui::TextBoxWithFilter& filter) override;

	protected:
		virtual const AssetTabConfig& GetConfig() const = 0;
		virtual void OnItemClicked(const AssetIdentificationType& asset);

		// For details panel. If invalid, displays a placeholder text instead of calling for further details rendering.
		virtual bool IsAssetValid(const AssetIdentificationType& asset) const = 0;
		// For grid/list panel. Optionally render a toolbar at the top.
		virtual void RenderToolbar();
		// For grid panel. Render a button that represents the asset. Return whether the button was clicked.
		virtual bool RenderGridItemButton(const AssetIdentificationType& asset, const std::string& assetName, const gui::Vec4& iconColor) = 0;
		virtual void RenderContextMenuItems(const AssetIdentificationType& asset);
		virtual void RenderDetailPanelContent(const AssetIdentificationType& asset, const std::string& assetName);
		virtual bool ShouldShowDetailPanel(const AssetIdentificationType& asset);

		// Callback params:
		//  1: The hash/name of the asset
		//  2: The display name of the asset
		virtual void ForEachAssetItem(const gui::TextBoxWithFilter& filter, std::function<void(const AssetIdentificationType&, const std::string&)> itemCallback) = 0;

		void SetSelectedAsset(std::optional<AssetIdentificationType> asset, const std::string& assetName = "");
		bool HasSelectedAsset() const;
		const AssetIdentificationType& GetSelectedAsset() const;
		const std::string& GetSelectedAssetName() const;

	private:
		std::optional<AssetIdentificationType> selectedAsset;
		std::string selectedAssetName;

		void RenderAssetView(const gui::TextBoxWithFilter& filter, float width);
		void RenderListItem(const AssetIdentificationType& asset, const std::string& name, bool isSelected, const gui::Vec4& iconColor);
		void RenderGridItem(const AssetIdentificationType& asset, const std::string& name, bool isSelected, float thumbSize, const gui::Vec4& iconColor);
		void RenderDetailPanel(const AssetIdentificationType& asset, const std::string& assetName);
	};

	// Unified asset tab template for AssetManager-based resources
	template<typename ResourceType>
	class GenericResourceAssetTab : public GenericAssetTab<size_t>
	{
	private:
		void ForEachAssetItem(const gui::TextBoxWithFilter& filter, std::function<void(const size_t&, const std::string&)> itemCallback) final;

		bool IsAssetValid(const size_t& asset) const final;
		bool RenderGridItemButton(const size_t& asset, const std::string& assetName, const gui::Vec4& iconColor) final;
	};

	// Asset tab for string-based items (prefabs, scripts, scenes, etc.)
	class GenericStringAssetTab : public GenericAssetTab<std::string>
	{
	private:
		void ForEachAssetItem(const gui::TextBoxWithFilter& filter, std::function<void(const std::string&, const std::string&)> itemCallback) final;

		bool IsAssetValid(const std::string& asset) const final;
		bool RenderGridItemButton(const std::string& asset, const std::string& assetName, const gui::Vec4& iconColor) final;

	protected:
		virtual std::vector<std::string> GetItemList() const = 0;
		virtual std::string GetDisplayName(const std::string& asset) const;
	};

}

#include "AssetBrowserCategories.ipp"
