#pragma once
#include "AssetBrowserCategories.h"

struct FileBrowserTab
	: editor::BaseAssetCategory
{
	const char* GetName() const override;
	const char* GetIdentifier() const override;
	void RenderBreadcrumb() override;
	void Render() override;
	void Initialize(const std::filesystem::path& initialPath);

private:
	FileSystem fileSystem;

	//VkDescriptorSet GetThumbnailDescriptor(std::filesystem::path::iterator::reference path);
	struct SpriteImportConfig
	{
		int spriteCount = 1; /**< Number of sprites in the sheet */
		std::filesystem::path targetPath; /**< Target path for the sprite import */
		bool showDialog = false; /**< Flag to show the import dialog */
		std::string spriteName; /**< Name of the sprite */
		bool isSpriteSheet = false; /**< Flag indicating if the import is a sprite sheet */
	};
	SpriteImportConfig spriteConfig;

	struct ThumbnailCache
	{
		//std::unordered_map<std::string, VkDescriptorSet> textureDescriptors; /**< Cache of texture descriptors */
		std::unordered_map<std::string, bool> loadingStatus; /**< Cache of loading statuses */
	};
	ThumbnailCache thumbnailCache;

	bool RenderDirectoryItem(const FileSystem::FileEntry& entry);
	void RenderItemLabel(const std::string& filename);
	bool RenderFileItem(const FileSystem::FileEntry& entry);
	void RenderItemContextMenu(const FileSystem::FileEntry& entry);

	void ShowSpriteSheetDialog();
	void ImportAsSpriteSheet(const std::filesystem::path& path, int spriteCount, const std::string& baseName);
	void ImportAsSprite(const std::filesystem::path& path, const std::string& name);
	std::filesystem::path CopyIntoWorkingDir(const std::filesystem::path& file);
};