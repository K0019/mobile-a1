#pragma once
#include "Editor/AssetBrowserCategories.h"
#include <array>

namespace editor {

	struct FileBrowserTab : BaseAssetCategory
	{
		const char* GetName() const override;
		const char* GetIdentifier() const override;
		void RenderBreadcrumb() override;
		void Render(const gui::TextBoxWithFilter& filter) override;
		void Initialize(const std::filesystem::path& initialPath);

		// Disable all standard toolbar controls - FileBrowserTab has its own filter UI
		ToolbarCapabilities GetToolbarCapabilities() const override { return { false, false, false }; }

		// Render custom filter controls in toolbar area
		void RenderToolbarExtras() override;

	private:
		// Extended file entry with size info
		struct FileEntry
		{
			std::string name;
			std::string fullPath;
			std::string extension;
			bool isDirectory = false;
			uint64_t size = 0;
		};

		// File list management
		void RefreshDirectory();
		std::vector<FileEntry> currentEntries_;
		int selectedIndex_ = -1;

		// Filter state
		std::array<char, 256> localFilterBuffer_{};
		int filterType_ = 0;  // 0=All, 1=3D, 2=Tex, 3=Dir
		bool showHiddenFiles_ = false;

		// Status feedback
		std::string status_;
		bool statusIsError_ = false;

		// Existing filesystem
		FileSystem fileSystem;

		// Rendering helpers
#ifdef IMGUI_ENABLED
		void RenderFileListContent(const gui::TextBoxWithFilter& filter);
		void RenderInfoPanel();
		void RenderItemContextMenu(const FileEntry& entry);
#endif

		// Helpers
		bool IsImportableFile(const std::string& ext) const;
		const char* GetFileTypeIcon(const std::string& ext) const;
		ImVec4 GetFileTypeColor(const std::string& ext) const;
		std::string FormatFileSize(uint64_t size) const;
		void ImportFile(const std::string& path);
	};

}
