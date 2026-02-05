#include "Editor/FileBrowserTab.h"
#include "Editor/AssetBrowserSettings.h"
#include "Editor/AssetCompilerWindow.h"
#include "Assets/AssetImporter.h"
#include "VFS/VFS.h"
#include "Editor/Import.h"
#include "Editor/Containers/GUICollection.h"
#include "Utilities/Logging.h"
#include "FilepathConstants.h"
#include "resource/asset_compiler_interface.h"

#include <algorithm>
#include <cctype>

namespace editor {

	const char* FileBrowserTab::GetName() const
	{
		return "File Browser";
	}

	const char* FileBrowserTab::GetIdentifier() const
	{
		return ICON_FA_FOLDER_TREE " File Browser";
	}

	void FileBrowserTab::Initialize(const std::filesystem::path& initialPath)
	{
		std::memset(localFilterBuffer_.data(), 0, localFilterBuffer_.size());
		fileSystem.Initialize(initialPath);
		RefreshDirectory();
	}

	void FileBrowserTab::RefreshDirectory()
	{
		currentEntries_.clear();
		selectedIndex_ = -1;

		std::error_code ec;
		auto currentPath = fileSystem.GetCurrentPath();

		if (!std::filesystem::exists(currentPath, ec) || !std::filesystem::is_directory(currentPath, ec))
			return;

		// Get local filter text (lowercase)
		std::string filterText(localFilterBuffer_.data());
		std::transform(filterText.begin(), filterText.end(), filterText.begin(),
			[](unsigned char c) { return static_cast<char>(std::tolower(c)); });

		try
		{
			for (const auto& entry : std::filesystem::directory_iterator(
				currentPath, std::filesystem::directory_options::skip_permission_denied, ec))
			{
				if (ec) continue;

				FileEntry fileEntry;
				fileEntry.name = entry.path().filename().string();
				fileEntry.fullPath = entry.path().string();
				fileEntry.isDirectory = entry.is_directory(ec);
				fileEntry.size = fileEntry.isDirectory ? 0 : (entry.is_regular_file(ec) ? entry.file_size(ec) : 0);
				fileEntry.extension = entry.path().extension().string();

				// Lowercase extension
				std::transform(fileEntry.extension.begin(), fileEntry.extension.end(),
					fileEntry.extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

				// Skip hidden files unless enabled
				if (!showHiddenFiles_ && !fileEntry.name.empty() && fileEntry.name[0] == '.')
					continue;

				// Apply type filter
				if (filterType_ == 1 && !fileEntry.isDirectory)  // 3D Models
				{
					if (fileEntry.extension != ".gltf" && fileEntry.extension != ".glb" &&
						fileEntry.extension != ".fbx" && fileEntry.extension != ".obj")
						continue;
				}
				else if (filterType_ == 2 && !fileEntry.isDirectory)  // Textures
				{
					if (fileEntry.extension != ".png" && fileEntry.extension != ".jpg" &&
						fileEntry.extension != ".jpeg" && fileEntry.extension != ".ktx" &&
						fileEntry.extension != ".ktx2" && fileEntry.extension != ".hdr" &&
						fileEntry.extension != ".tga" && fileEntry.extension != ".bmp")
						continue;
				}
				else if (filterType_ == 3 && !fileEntry.isDirectory)  // Folders only
				{
					continue;
				}

				// Apply text filter
				if (!filterText.empty())
				{
					std::string nameLower = fileEntry.name;
					std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(),
						[](unsigned char c) { return static_cast<char>(std::tolower(c)); });
					if (nameLower.find(filterText) == std::string::npos)
						continue;
				}

				currentEntries_.push_back(std::move(fileEntry));
			}
		}
		catch (const std::exception&)
		{
			// Silently handle permission errors
		}

		// Sort: directories first, then alphabetically
		std::sort(currentEntries_.begin(), currentEntries_.end(),
			[](const FileEntry& a, const FileEntry& b)
			{
				if (a.isDirectory != b.isDirectory)
					return a.isDirectory > b.isDirectory;
				return a.name < b.name;
			});
	}

	bool FileBrowserTab::IsImportableFile(const std::string& ext) const
	{
		return AssetImporter::FiletypeSupported(ext);
	}

	const char* FileBrowserTab::GetFileTypeIcon(const std::string& ext) const
	{
		if (ext == ".gltf" || ext == ".glb") return ICON_FA_CUBES;
		if (ext == ".fbx") return ICON_FA_CUBE;
		if (ext == ".obj") return ICON_FA_SHAPES;
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") return ICON_FA_IMAGE;
		if (ext == ".ktx" || ext == ".ktx2") return ICON_FA_FILE_IMAGE;
		if (ext == ".hdr") return ICON_FA_SUN;
		if (ext == ".lua") return ICON_FA_SCROLL;
		if (ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".hina_sl") return ICON_FA_MICROCHIP;
		if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") return ICON_FA_MUSIC;
		if (ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".webm") return ICON_FA_VIDEO;
		if (ext == ".scene") return ICON_FA_FILM;
		if (ext == ".prefab") return ICON_FA_BOX;
		if (ext == ".json") return ICON_FA_CODE;
		return ICON_FA_FILE;
	}

	ImVec4 FileBrowserTab::GetFileTypeColor(const std::string& ext) const
	{
		if (ext == ".gltf" || ext == ".glb") return ImVec4(0.4f, 0.9f, 0.4f, 1.0f);  // Green
		if (ext == ".fbx") return ImVec4(0.9f, 0.6f, 0.2f, 1.0f);  // Orange
		if (ext == ".obj") return ImVec4(0.6f, 0.8f, 0.4f, 1.0f);  // Light green
		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp") return ImVec4(0.4f, 0.7f, 1.0f, 1.0f);  // Blue
		if (ext == ".ktx" || ext == ".ktx2") return ImVec4(0.8f, 0.5f, 0.9f, 1.0f);  // Purple
		if (ext == ".hdr") return ImVec4(1.0f, 0.85f, 0.4f, 1.0f);  // Yellow
		if (ext == ".lua") return ImVec4(0.4f, 0.6f, 1.0f, 1.0f);  // Blue
		if (ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".hina_sl") return ImVec4(0.9f, 0.4f, 0.6f, 1.0f);  // Pink
		if (ext == ".wav" || ext == ".mp3" || ext == ".ogg" || ext == ".flac") return ImVec4(0.9f, 0.5f, 0.9f, 1.0f);  // Magenta
		if (ext == ".mp4" || ext == ".avi" || ext == ".mov" || ext == ".webm") return ImVec4(0.7f, 0.3f, 0.9f, 1.0f);  // Purple
		return ImVec4(0.6f, 0.6f, 0.6f, 1.0f);  // Gray
	}

	std::string FileBrowserTab::FormatFileSize(uint64_t size) const
	{
		if (size < 1024)
			return std::to_string(size) + " B";
		else if (size < 1024 * 1024)
			return std::to_string(size / 1024) + " KB";
		else
			return std::to_string(size / (1024 * 1024)) + " MB";
	}

	void FileBrowserTab::ImportFile(const std::string& path)
	{
		std::string vfsPath = VFS::ConvertPhysicalToVirtual(path);
		if (vfsPath.empty())
		{
			std::filesystem::path fsPath(path);
			std::filesystem::path resultPath;
			auto result = import::ImportToAssets(fsPath, &resultPath);
			if (result == import::IMPORT_RESULT::SUCCESS || result == import::IMPORT_RESULT::ALREADY_IMPORTED)
			{
				vfsPath = VFS::ConvertPhysicalToVirtual(resultPath.string());
				status_ = "Copied: " + fsPath.filename().string();
				statusIsError_ = false;
			}
			else
			{
				status_ = "Failed: " + fsPath.filename().string();
				statusIsError_ = true;
				return;
			}
		}

		AssetImporter::Import(vfsPath);
		status_ = "Imported: " + std::filesystem::path(path).filename().string();
		statusIsError_ = false;
	}

	void FileBrowserTab::RenderBreadcrumb()
	{
#ifdef IMGUI_ENABLED
		// Use minimal padding for icon-only navigation buttons
		constexpr float buttonSize = 28.0f;
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 4));

		// Navigation buttons
		ImGui::BeginDisabled(!fileSystem.CanNavigateBack());
		if (ImGui::Button(ICON_FA_ARROW_LEFT, ImVec2(buttonSize, buttonSize)))
		{
			fileSystem.NavigateBack();
			RefreshDirectory();
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("Back");

		ImGui::SameLine();

		ImGui::BeginDisabled(!fileSystem.CanNavigateForward());
		if (ImGui::Button(ICON_FA_ARROW_RIGHT, ImVec2(buttonSize, buttonSize)))
		{
			fileSystem.NavigateForward();
			RefreshDirectory();
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("Forward");

		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_ARROW_UP, ImVec2(buttonSize, buttonSize)))
		{
			fileSystem.NavigateUp();
			RefreshDirectory();
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("Up");

		ImGui::SameLine();

		if (ImGui::Button(ICON_FA_ROTATE_RIGHT, ImVec2(buttonSize, buttonSize)))
			RefreshDirectory();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) ImGui::SetTooltip("Refresh");

		ImGui::PopStyleVar();  // Restore FramePadding

		ImGui::SameLine();

		// Path display
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", fileSystem.GetCurrentPath().string().c_str());
#endif
	}

	void FileBrowserTab::RenderToolbarExtras()
	{
#ifdef IMGUI_ENABLED
		gui::SameLine();  // Use default ItemSpacing after breadcrumb

		ImGui::SetNextItemWidth(150);  // Wider to account for frame padding
		if (ImGui::InputTextWithHint("##LocalFilter", ICON_FA_FILTER " Filter...", localFilterBuffer_.data(), localFilterBuffer_.size()))
			RefreshDirectory();

		gui::SameLine();

		ImGui::SetNextItemWidth(80);  // Wider for combo dropdown
		const char* filterTypes[] = { "All", "3D", "Tex", "Dir" };
		if (ImGui::Combo("##TypeFilter", &filterType_, filterTypes, IM_ARRAYSIZE(filterTypes)))
			RefreshDirectory();

		gui::SameLine();

		if (ImGui::Checkbox("Hidden", &showHiddenFiles_))
			RefreshDirectory();
		if (ImGui::IsItemHovered()) ImGui::SetTooltip("Show hidden files (starting with '.')");

		gui::SameLine();
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "| %zu items", currentEntries_.size());
#endif
	}

	void FileBrowserTab::Render([[maybe_unused]] const gui::TextBoxWithFilter& filter)
	{
#ifdef IMGUI_ENABLED
		// Status bar
		if (!status_.empty())
		{
			ImVec4 statusColor = statusIsError_ ? ImVec4(1.0f, 0.4f, 0.4f, 1.0f) : ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
			ImGui::TextColored(statusColor, "%s", status_.c_str());
			ImGui::SameLine();
			if (ImGui::SmallButton(ICON_FA_XMARK "##clearstatus"))
				status_.clear();
			ImGui::Separator();
		}

		// Side-by-side layout: file list on left, info panel on right
		bool hasSelection = (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(currentEntries_.size()));
		float infoPanelWidth = hasSelection ? 220.0f : 0.0f;
		float availableWidth = ImGui::GetContentRegionAvail().x;

		// File list (left side)
		ImGui::BeginChild("FileListContent", ImVec2(availableWidth - infoPanelWidth, 0), ImGuiChildFlags_None);
		RenderFileListContent(filter);
		ImGui::EndChild();

		// Info panel (right side)
		if (hasSelection)
		{
			ImGui::SameLine();
			ImGui::BeginChild("FileInfoPanel", ImVec2(0, 0), ImGuiChildFlags_Borders);
			RenderInfoPanel();
			ImGui::EndChild();
		}
#endif
	}

#ifdef IMGUI_ENABLED
	void FileBrowserTab::RenderFileListContent(const gui::TextBoxWithFilter& filter)
	{
		for (int i = 0; i < static_cast<int>(currentEntries_.size()); ++i)
		{
			const auto& entry = currentEntries_[static_cast<size_t>(i)];

			// Apply main filter from toolbar
			if (!filter.PassFilter(entry.name))
				continue;

			ImGui::PushID(i);

			bool isSelected = (i == selectedIndex_);
			bool isImportable = !entry.isDirectory && IsImportableFile(entry.extension);
			bool isCompilable = !entry.isDirectory && Resource::AssetCompilerInterface::IsCompilableExtension(entry.extension);

			// Highlight importable files with subtle background
			if (isImportable && !isSelected)
			{
				ImVec2 cursorPos = ImGui::GetCursorScreenPos();
				ImVec2 rowSize = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetTextLineHeightWithSpacing());
				ImGui::GetWindowDrawList()->AddRectFilled(
					cursorPos,
					ImVec2(cursorPos.x + rowSize.x, cursorPos.y + rowSize.y),
					IM_COL32(40, 80, 40, 40)  // Subtle green tint
				);
			}

			// Icon
			if (entry.isDirectory)
				ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), ICON_FA_FOLDER);
			else
				ImGui::TextColored(GetFileTypeColor(entry.extension), "%s", GetFileTypeIcon(entry.extension));

			ImGui::SameLine();

			// Selectable
			if (ImGui::Selectable(entry.name.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick))
			{
				selectedIndex_ = i;

				if (ImGui::IsMouseDoubleClicked(0))
				{
					if (entry.isDirectory)
					{
						fileSystem.NavigateTo(entry.fullPath);
						RefreshDirectory();
					}
					else if (isImportable)
					{
						ImportFile(entry.fullPath);
					}
				}
			}

			// Context menu
			if (ImGui::BeginPopupContextItem())
			{
				RenderItemContextMenu(entry);
				ImGui::EndPopup();
			}

			// Right side: import badge + file size
			float rightOffset = 70.0f;
			if (isImportable)
				rightOffset += 25.0f;

			if (!entry.isDirectory)
			{
				// Import indicator
				if (isImportable)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - rightOffset);
					if (isCompilable)
						ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), ICON_FA_CIRCLE_CHECK);
					else
						ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), ICON_FA_CIRCLE_PLUS);
				}

				// File size
				if (entry.size > 0)
				{
					ImGui::SameLine(ImGui::GetWindowWidth() - 70);
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", FormatFileSize(entry.size).c_str());
				}
			}

			ImGui::PopID();
		}
	}

	void FileBrowserTab::RenderInfoPanel()
	{
		// Guard against stale selection (can happen if RefreshDirectory() was called during RenderFileListContent())
		if (selectedIndex_ < 0 || selectedIndex_ >= static_cast<int>(currentEntries_.size()))
			return;

		const auto& selected = currentEntries_[static_cast<size_t>(selectedIndex_)];
		bool isImportable = !selected.isDirectory && IsImportableFile(selected.extension);
		bool isCompilable = !selected.isDirectory && Resource::AssetCompilerInterface::IsCompilableExtension(selected.extension);

		// Header with icon and name
		if (selected.isDirectory)
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.4f, 1.0f), ICON_FA_FOLDER);
		else
			ImGui::TextColored(GetFileTypeColor(selected.extension), "%s", GetFileTypeIcon(selected.extension));

		ImGui::SameLine();
		ImGui::TextWrapped("%s", selected.name.c_str());

		// Close button
		ImGui::SameLine(ImGui::GetContentRegionAvail().x - 10);
		if (ImGui::SmallButton(ICON_FA_XMARK "##close"))
			selectedIndex_ = -1;

		ImGui::Separator();

		// File details
		if (selected.isDirectory)
		{
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Type:");
			ImGui::SameLine();
			ImGui::Text("Folder");

			// Count items in folder
			std::error_code ec;
			int itemCount = 0;
			for ([[maybe_unused]] const auto& _ : std::filesystem::directory_iterator(selected.fullPath, ec))
				if (!ec) itemCount++;

			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Items:");
			ImGui::SameLine();
			ImGui::Text("%d", itemCount);
		}
		else
		{
			// Type
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Type:");
			ImGui::SameLine();
			ImGui::Text("%s", selected.extension.empty() ? "File" : selected.extension.c_str() + 1);

			// Size
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Size:");
			ImGui::SameLine();
			ImGui::Text("%s", FormatFileSize(selected.size).c_str());

			// Import status
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status:");
			ImGui::SameLine();
			if (isCompilable)
				ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.3f, 1.0f), ICON_FA_CIRCLE_CHECK " Compilable");
			else if (isImportable)
				ImGui::TextColored(ImVec4(0.5f, 0.8f, 1.0f, 1.0f), ICON_FA_CIRCLE_PLUS " Direct Import");
			else
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), ICON_FA_CIRCLE_XMARK " Not Supported");
		}

		// Full path (collapsible)
		ImGui::Spacing();
		if (ImGui::TreeNode("Path"))
		{
			ImGui::TextWrapped("%s", selected.fullPath.c_str());
			ImGui::TreePop();
		}

		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();

		// Actions
		if (selected.isDirectory)
		{
			if (ImGui::Button(ICON_FA_FOLDER_OPEN " Open", ImVec2(-1, 0)))
			{
				fileSystem.NavigateTo(selected.fullPath);
				RefreshDirectory();
			}
		}
		else
		{
			// Import button (highlighted for importable files)
			if (isImportable)
			{
				if (isCompilable)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 0.6f));
					if (ImGui::Button(ICON_FA_GEARS " Compile & Import", ImVec2(-1, 0)))
						ImportFile(selected.fullPath);
					ImGui::PopStyleColor();

					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Will convert to engine format");
				}
				else
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.59f, 0.98f, 0.6f));
					if (ImGui::Button(ICON_FA_FILE_IMPORT " Import", ImVec2(-1, 0)))
						ImportFile(selected.fullPath);
					ImGui::PopStyleColor();
				}

				ImGui::Spacing();
			}

			if (ImGui::Button(ICON_FA_COPY " Copy Path", ImVec2(-1, 0)))
				ImGui::SetClipboardText(selected.fullPath.c_str());

#ifdef _WIN32
			if (ImGui::Button(ICON_FA_FOLDER_OPEN " Show in Explorer", ImVec2(-1, 0)))
			{
				std::string command = "explorer /select,\"" + selected.fullPath + "\"";
				system(command.c_str());
			}
#endif
		}
	}

	void FileBrowserTab::RenderItemContextMenu(const FileEntry& entry)
	{
		if (entry.isDirectory)
		{
			if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Open"))
			{
				fileSystem.NavigateTo(entry.fullPath);
				RefreshDirectory();
			}
		}
		else
		{
			bool isCompilable = Resource::AssetCompilerInterface::IsCompilableExtension(entry.extension);

			if (isCompilable)
			{
				if (ImGui::BeginMenu(ICON_FA_HAMMER " Recompile"))
				{
					std::string vfsPath = VFS::ConvertPhysicalToVirtual(entry.fullPath);

					if (ImGui::MenuItem("Windows (BC7)"))
					{
						auto* compilerWindow = editor::AssetCompilerWindow::GetInstance();
						if (!compilerWindow)
						{
							editor::CreateGuiWindow<editor::AssetCompilerWindow>();
							compilerWindow = editor::AssetCompilerWindow::GetInstance();
						}
						if (compilerWindow)
							compilerWindow->CompileAsset(vfsPath, "windows");
					}
					if (ImGui::MenuItem("Android (ASTC)"))
					{
						auto* compilerWindow = editor::AssetCompilerWindow::GetInstance();
						if (!compilerWindow)
						{
							editor::CreateGuiWindow<editor::AssetCompilerWindow>();
							compilerWindow = editor::AssetCompilerWindow::GetInstance();
						}
						if (compilerWindow)
							compilerWindow->CompileAsset(vfsPath, "android");
					}
					ImGui::EndMenu();
				}
				ImGui::Separator();
			}

			if (IsImportableFile(entry.extension))
			{
				if (ImGui::MenuItem(ICON_FA_FILE_IMPORT " Import"))
					ImportFile(entry.fullPath);
				ImGui::Separator();
			}
		}

		if (ImGui::MenuItem(ICON_FA_COPY " Copy Path"))
			ImGui::SetClipboardText(entry.fullPath.c_str());

#ifdef _WIN32
		if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN " Show in Explorer"))
		{
			std::string command = "explorer /select,\"" + entry.fullPath + "\"";
			system(command.c_str());
		}
#endif
	}
#endif

}
