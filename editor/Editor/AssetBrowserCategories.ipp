#include "AssetBrowserCategories.h"

namespace editor {

	template<typename AssetIdentificationType>
	const char* GenericAssetTab<AssetIdentificationType>::GetName() const
	{
		return GetConfig().name;
	}
	template<typename AssetIdentificationType>
	const char* GenericAssetTab<AssetIdentificationType>::GetIdentifier() const
	{
		return GetConfig().identifier;
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::OnItemClicked([[maybe_unused]] const AssetIdentificationType& asset)
	{
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::Render(const gui::TextBoxWithFilter& filter)
	{
		const auto& config = GetConfig();
		float detailPanelWidth = HasSelectedAsset() ? config.detailPanelWidth : 0.0f;
		float availableWidth = gui::GetAvailableContentRegion().x;

		// Asset grid/list (left side)
		RenderAssetView(filter, availableWidth - detailPanelWidth);

		// Detail panel (right side)
		if (HasSelectedAsset()) // Check again since RenderAssetView() may have deselected the asset
		{
			gui::SameLine();
			gui::Child assetDetailsChild{ "AssetDetails", gui::Vec2{}, gui::FLAG_CHILD::BORDERS };
			RenderDetailPanel(GetSelectedAsset(), GetSelectedAssetName());
		}
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderAssetView(const gui::TextBoxWithFilter& filter, float width)
	{
		gui::Child assetGridChild{ "AssetGrid", gui::Vec2{ width, 0.0f } };

		RenderToolbar();

		gui::NewGridHelper grid{ AssetBrowserSettings::thumbnailSize };

		// Delegate to implementation to give us all the assets one at a time
		switch (AssetBrowserSettings::viewMode)
		{
		case AssetBrowserSettings::ViewMode::List:
			ForEachAssetItem(filter, [this](const AssetIdentificationType& asset, const std::string& assetName) -> void {
				gui::SetID id{ asset };
				RenderListItem(asset, assetName, (HasSelectedAsset() ? asset == GetSelectedAsset() : false), GetConfig().iconColor);
			});
			break;
		case AssetBrowserSettings::ViewMode::Grid:
			ForEachAssetItem(filter, [this, &grid](const AssetIdentificationType& asset, const std::string& assetName) -> void {
				gui::GridItem item{ grid.Item(assetName) };
				RenderGridItem(asset, assetName, (HasSelectedAsset() ? asset == GetSelectedAsset() : false), AssetBrowserSettings::thumbnailSize, GetConfig().iconColor);
			});
			break;
		}
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderToolbar()
	{
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderListItem(const AssetIdentificationType& asset, const std::string& name, bool isSelected, const gui::Vec4& iconColor)
	{
		{
			gui::SetStyleColor headerColor{ gui::FLAG_STYLE_COLOR::HEADER, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.3f } };
			if (gui::Selectable("##sel", isSelected, gui::Vec2{ 0.0f, 20.0f }))
				SetSelectedAsset(isSelected ? std::nullopt : std::make_optional(asset));
		}

		if (gui::ItemContextMenu contextMenu{ "ctx" })
			RenderContextMenuItems(asset);

		if (const char* payloadType{ GetConfig().payloadType })
			gui::PayloadSource{ payloadType, asset };

		gui::SameLine(0.0f, 0.0f);
		gui::SetDrawCursorPosX(8.0f);
		gui::TextColored(iconColor, "%s", GetConfig().icon);
		gui::SameLine();
		gui::TextFormatted("%s", name.c_str());
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderGridItem(const AssetIdentificationType& asset, const std::string& assetName, bool isSelected, float thumbSize, const gui::Vec4& iconColor)
	{
		const auto& config = GetConfig();
		gui::Group group{};

		{
			gui::SetStyleColor buttonColor{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.6f }, isSelected };
			bool clicked{ RenderGridItemButton(asset, assetName, iconColor) };
			if (clicked)
			{
				SetSelectedAsset(asset);
				OnItemClicked(asset);
			}
		}

		if (gui::ItemContextMenu contextMenu{ "ctx" })
			RenderContextMenuItems(asset);

		if (const char* payloadType{ GetConfig().payloadType })
			gui::PayloadSource{ payloadType, asset };
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderDetailPanel(const AssetIdentificationType& asset, const std::string& assetName)
	{
		if (!IsAssetValid(asset))
		{
			gui::TextColored(gui::Vec4{ 1.0f, 0.4f, 0.4f, 1.0f }, "%s not found", GetConfig().name);
			return;
		}

		gui::TextColored(GetConfig().iconColor, "%s", GetConfig().icon);
		gui::SameLine();
		gui::TextWrapped("%s", assetName.c_str());
		gui::SameLine(gui::GetAvailableContentRegion().x - 10.0f);
		if (gui::SmallButton{ ICON_FA_XMARK })
		{
			SetSelectedAsset(std::nullopt);
			return;
		}

		gui::Separator();

		if (ShouldShowDetailPanel(asset))
			RenderDetailPanelContent(asset, assetName);

		if (const char* payloadType{ GetConfig().payloadType })
		{
			gui::Button{ ICON_FA_HAND" Drag to assign", gui::Vec2{ -1.0f, 0.0f } };
			gui::PayloadSource{ payloadType, asset, (GetConfig().name + assetName).c_str(), gui::FLAG_PAYLOAD_SOURCE::ALLOW_NULL_ID};
		}
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderContextMenuItems([[maybe_unused]] const AssetIdentificationType& asset)
	{
	}
	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::RenderDetailPanelContent([[maybe_unused]] const AssetIdentificationType& asset, [[maybe_unused]] const std::string& assetName)
	{
	}
	template<typename AssetIdentificationType>
	bool GenericAssetTab<AssetIdentificationType>::ShouldShowDetailPanel([[maybe_unused]] const AssetIdentificationType& asset)
	{
		return true;
	}

	template<typename AssetIdentificationType>
	void GenericAssetTab<AssetIdentificationType>::SetSelectedAsset(std::optional<AssetIdentificationType> asset, const std::string& assetName)
	{
		selectedAsset = asset;
		if (asset.has_value())
			selectedAssetName = assetName;
		else
			selectedAssetName.clear();
	}
	template<typename AssetIdentificationType>
	bool GenericAssetTab<AssetIdentificationType>::HasSelectedAsset() const
	{
		return selectedAsset.has_value();
	}
	template<typename AssetIdentificationType>
	const AssetIdentificationType& GenericAssetTab<AssetIdentificationType>::GetSelectedAsset() const
	{
		return selectedAsset.value();
	}
	template<typename AssetIdentificationType>
	const std::string& GenericAssetTab<AssetIdentificationType>::GetSelectedAssetName() const
	{
		return selectedAssetName;
	}

	template<typename ResourceType>
	void GenericResourceAssetTab<ResourceType>::ForEachAssetItem(const gui::TextBoxWithFilter& filter, std::function<void(const size_t&, const std::string&)> itemCallback)
	{
		for (const auto& [hash, resourceRef] : ST<AssetManager>::Get()->Editor_GetContainer<ResourceType>().Editor_GetAllResources())
		{
			const std::string& assetName = *ST<AssetManager>::Get()->Editor_GetName(hash);
			if (!filter.PassFilter(assetName))
				continue;

			itemCallback(hash, assetName);
		}
	}

	template<typename ResourceType>
	bool GenericResourceAssetTab<ResourceType>::IsAssetValid(const size_t& asset) const
	{
		return ST<AssetManager>::Get()->Editor_GetName(asset) != nullptr;
	}

	template<typename ResourceType>
	bool GenericResourceAssetTab<ResourceType>::RenderGridItemButton(const size_t& asset, const std::string& assetName, const gui::Vec4& iconColor)
	{
		// If has thumbnail, render image button
		if (GetConfig().hasThumbnails)
			if (uint64_t thumbId{ ThumbnailCache::Get().GetThumbnail(asset, GetConfig().thumbnailType, assetName) })
			{
				return gui::ImageButton{
					"##item",
					thumbId,
					gui::Vec2{ AssetBrowserSettings::thumbnailSize, AssetBrowserSettings::thumbnailSize },
					gui::Vec2{ 0.0f, 1.0f }, gui::Vec2{ 1.0f, 0.0f }
				};
			}

		// If no or invalid thumbnail, render normal button
		gui::SetStyleColor textColor{ gui::FLAG_STYLE_COLOR::TEXT, iconColor };
		return gui::Button{
			GetConfig().icon,
			gui::Vec2{ AssetBrowserSettings::thumbnailSize, AssetBrowserSettings::thumbnailSize }
		};
	}

}