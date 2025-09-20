#include "FileBrowserTab.h"
#include "ResourceManager.h"
#include "Import.h"



const char* FileBrowserTab::GetName() const
{
    return "File Browser";
}

const char* FileBrowserTab::GetIdentifier() const
{
    return ICON_FA_INBOX" File Browser";
}

void FileBrowserTab::RenderBreadcrumb()
{
    // Back button
    if (ImGui::Button(ICON_FA_ARROW_LEFT) && fileSystem.CanNavigateBack())
    {
        fileSystem.NavigateBack();
    }
    ImGui::SameLine();

    // Forward button
    if (ImGui::Button(ICON_FA_ARROW_RIGHT) && fileSystem.CanNavigateForward())
    {
        fileSystem.NavigateForward();
    }
    ImGui::SameLine();

    // Up button
    if (ImGui::Button(ICON_FA_ARROW_UP))
    {
        fileSystem.NavigateUp();
    }
    ImGui::SameLine();

    // Begin path bar group
    ImGui::BeginGroup();

    // Build path components from root to current
    std::vector<std::filesystem::path> pathComponents;
    std::filesystem::path currPath{ fileSystem.GetCurrentPath() };
    std::filesystem::path rootPath{ fileSystem.GetRootPath() };
    for (std::filesystem::path iterPath{ currPath }; iterPath != rootPath; iterPath = iterPath.parent_path())
        pathComponents.push_back(iterPath);
    pathComponents.push_back(rootPath);

    // Render path components
    bool isFirst{ true };
    for (auto pathIter{ pathComponents.rbegin() }, pathEnd{ pathComponents.rend() }; pathIter != pathEnd; ++pathIter)
    {
        // Render chevron
        if (!isFirst)
        {
            gui::SameLine(0.0f, 0.0f);
            gui::TextUnformatted(ICON_FA_CHEVRON_RIGHT);
            gui::SameLine();
        }
        isFirst = false;

        gui::SetID id{ pathIter->string().c_str() };

        // Style for path segment button
        bool isCurrentDir{ *pathIter == currPath };
        gui::SetStyleColor buttonStyle{ gui::FLAG_STYLE_COLOR::BUTTON, gui::Vec4{} };
        gui::SetStyleColor buttonHoveredStyle{ gui::FLAG_STYLE_COLOR::BUTTON_HOVERED, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.31f } };
        gui::SetStyleColor buttonActiveStyle{ gui::FLAG_STYLE_COLOR::BUTTON_ACTIVE, gui::Vec4{ 0.26f, 0.59f, 0.98f, 0.5f } };
        gui::SetStyleColor textStyle{ gui::FLAG_STYLE_COLOR::TEXT, gui::Vec4{ 0.26f, 0.59f, 0.98f, 1.0f }, isCurrentDir };

        // Render button
        std::string displayName{ pathIter->filename().string() };
        if (displayName.empty()) // Could be empty if we're at root
            displayName = pathIter->string();
        if (gui::Button button{ displayName.c_str() })
            fileSystem.NavigateTo(*pathIter);

        gui::UnsetStyleColor popTextStyle{ isCurrentDir };

        if (ImGui::BeginPopupContextItem("path_context_menu"))
        {
            if (ImGui::MenuItem("Copy Path"))
            {
                ImGui::SetClipboardText(pathIter->string().c_str());
            }
            ImGui::EndPopup();
        }

        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::Text("%s", pathIter->string().c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::EndGroup();
}

void FileBrowserTab::Initialize(const std::filesystem::path& initialPath)
{
    fileSystem.Initialize(initialPath);
}

void FileBrowserTab::Render()
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;

    // Main file view
    ImGui::BeginChild("FileView", ImVec2(0, 0), true);
    {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
        if (columnsCount < 1) columnsCount = 1;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        auto entries = fileSystem.ScanDirectory(fileSystem.GetCurrentPath());

        size_t itemCount = 0;
        for (const auto& entry : entries)
        {
            if (!MatchesFilter(entry.filename))
            {
                continue;
            }

            ImGui::BeginGroup();
            ImGui::PushID(entry.fullPath.string().c_str());

            bool clicked = false;

            // Render the item and check for right-click menu
            if (entry.isDirectory)
            {
                clicked = RenderDirectoryItem(entry);
                RenderItemLabel(entry.filename);
            }
            else
            {
                clicked = RenderFileItem(entry);
                if (ImGui::BeginPopupContextItem(entry.filename.c_str()))
                {
                    RenderItemContextMenu(entry);
                    ImGui::EndPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", entry.filename.c_str());
                    ImGui::Text("Type: %s", entry.fileType.c_str());
                    ImGui::Text("Path: %s", entry.fullPath.string().c_str());
                    ImGui::EndTooltip();
                }

                RenderItemLabel(entry.filename);
            }

            if (clicked && entry.isDirectory)
            {
                fileSystem.NavigateTo(entry.fullPath);
            }

            ImGui::PopID();
            ImGui::EndGroup();

            if ((itemCount + 1) % columnsCount != 0 && itemCount < entries.size() - 1)
            {
                ImGui::SameLine();
            }
            itemCount++;
        }

        ImGui::PopStyleVar(2);
    }
    ImGui::EndChild();

    ShowSpriteSheetDialog();
}

bool FileBrowserTab::RenderDirectoryItem(const FileSystem::FileEntry& entry)
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    bool clicked = ImGui::Button((ICON_FA_FOLDER"##" + entry.filename).c_str(),
        ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    ImGui::PopStyleColor();
    return clicked;
}

void FileBrowserTab::RenderItemLabel(const std::string& filename)
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;

    std::string displayName = filename;
    ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());

    if (textSize.x > THUMBNAIL_SIZE)
    {
        while (textSize.x > THUMBNAIL_SIZE && displayName.length() > 3)
        {
            displayName = displayName.substr(0, displayName.length() - 4) + "...";
            textSize = ImGui::CalcTextSize(displayName.c_str());
        }
    }

    float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
    ImGui::TextUnformatted(displayName.c_str());
}

bool FileBrowserTab::RenderFileItem(const FileSystem::FileEntry& entry)
{
    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;

    bool clicked;
    if (entry.fileType == "texture")
    {
        /*VkDescriptorSet descriptor = GetThumbnailDescriptor(entry.fullPath);
        if (descriptor)
        {
            clicked = ImGui::ImageButton(("##" + entry.filename).c_str(),
                0,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        }*/

        {
            clicked = ImGui::Button((ICON_FA_IMAGE"##" + entry.filename).c_str(),
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        }
    }
    else if (entry.fileType == "audio")
    {
        clicked = ImGui::Button((ICON_FA_MUSIC"##" + entry.filename).c_str(),
            ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    else if (entry.fileType == "font")
    {
        clicked = ImGui::Button((ICON_FA_FONT"##" + entry.filename).c_str(),
            ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    else
    {
        clicked = ImGui::Button((ICON_FA_FILE"##" + entry.filename).c_str(),
            ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    return clicked;
}

void FileBrowserTab::RenderItemContextMenu(const FileSystem::FileEntry& entry)
{
    if (entry.fileType == "texture")
    {
        if (ImGui::MenuItem(ICON_FA_FILE_IMPORT" Import as Sprite"))
        {
            spriteConfig.showDialog = true;
            spriteConfig.targetPath = entry.fullPath;
            spriteConfig.spriteName = entry.fullPath.stem().string();
            spriteConfig.isSpriteSheet = false;
        }
        if (ImGui::MenuItem(ICON_FA_FILE_IMPORT" Import as Sprite Sheet"))
        {
            spriteConfig.showDialog = true;
            spriteConfig.targetPath = entry.fullPath;
            spriteConfig.spriteName = entry.fullPath.stem().string();
            spriteConfig.isSpriteSheet = true;
            spriteConfig.spriteCount = 1;
        }
        ImGui::Separator();
    }
    else if (entry.fileType == "fbx")
    {
        if (gui::MenuItem(ICON_FA_FILE_IMPORT" Import as Mesh"))
            ResourceManager::Import(RESOURCE_TYPE::MESH, entry.filename, entry.fullPath.string());
    }

    if (ImGui::MenuItem(ICON_FA_COPY" Copy Path"))
    {
        ImGui::SetClipboardText(entry.fullPath.string().c_str());
    }
    if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN" Show in Explorer"))
    {
        std::string command = "explorer /select,\"" + entry.fullPath.string() + "\"";
        system(command.c_str());
    }
}

VkDescriptorSet FileBrowserTab::GetThumbnailDescriptor(std::filesystem::path::iterator::reference path)
{
    std::string pathStr = path.string();

    // Check if we already have the descriptor
    auto it = thumbnailCache.textureDescriptors.find(pathStr);
    if (it != thumbnailCache.textureDescriptors.end())
    {
        return it->second;
    }

    // If we haven't tried loading it yet, try now
    if (!thumbnailCache.loadingStatus[pathStr])
    {
        thumbnailCache.loadingStatus[pathStr] = true;

        // Attempt to load the thumbnail
        std::string relativePath = ST<Filepaths>::Get()->MakeRelativeToWorkingDir(path);
        /*if (!ResourceManagerOld::TextureExists(relativePath))
        {
            ResourceManagerOld::LoadTexture(path.string(), relativePath);
        }

        // If texture was loaded successfully, cache its descriptor
        if (ResourceManagerOld::TextureExists(relativePath))
        {
            /*const Texture& tex = ResourceManagerOld::GetTexture(relativePath);
            thumbnailCache.textureDescriptors[pathStr] = tex.ImGui_handle;
            return tex.ImGui_handle;#1#
        }*/
    }
    return nullptr;
}

void FileBrowserTab::ShowSpriteSheetDialog()
{
    if (!spriteConfig.showDialog) return;

    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    const char* title = spriteConfig.isSpriteSheet ? "Import Sprite Sheet" : "Import Sprite";
    ImGui::SetNextWindowSize(ImVec2(400.0f, spriteConfig.isSpriteSheet ? 250.0f : 150.0f), ImGuiCond_FirstUseEver);

    if (ImGui::Begin(title, &spriteConfig.showDialog,
        ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
    {

        // File information
        ImGui::TextWrapped("File: %s", spriteConfig.targetPath.filename().string().c_str());
        ImGui::Separator();

        // If it's a sprite sheet, show sprite count and assumptions
        if (spriteConfig.isSpriteSheet)
        {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Sprite Sheet Assumptions:");
            ImGui::BeginChild("Assumptions", ImVec2(0, 80), true);
            ImGui::BulletText("All sprites are arranged horizontally");
            ImGui::BulletText("All sprites have equal width");
            ImGui::BulletText("All sprites use the full height of the image");
            ImGui::BulletText("No padding between sprites");
            ImGui::EndChild();

            ImGui::Spacing();

            // Sprite count input (only for sprite sheets)
            ImGui::SetNextItemWidth(200);
            ImGui::InputInt("Number of Sprites", &spriteConfig.spriteCount);
            spriteConfig.spriteCount = std::max(1, spriteConfig.spriteCount);
        }

        // Common fields for both modes
        static char nameBuffer[256];
        strncpy_s(nameBuffer, spriteConfig.spriteName.c_str(), sizeof(nameBuffer) - 1);
        if (ImGui::InputText("Sprite Name", nameBuffer, sizeof(nameBuffer)))
        {
            spriteConfig.spriteName = nameBuffer;
        }

        std::string relativePath = ST<Filepaths>::Get()->MakeRelativeToWorkingDir(spriteConfig.targetPath);
        // Show dimensions info
        /*if (ResourceManagerOld::TextureExists(relativePath))
        {
            /*const Texture& tex = ResourceManagerOld::GetTexture(relativePath);
            if (spriteConfig.isSpriteSheet)
            {
                int spriteWidth = tex.extent.width / spriteConfig.spriteCount;
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "Each sprite will be %dx%d pixels",
                    spriteWidth, tex.extent.height);

                // Show naming convention for sprite sheet
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "Sprites will be named: %s_0, %s_1, ...",
                    spriteConfig.spriteName.c_str(),
                    spriteConfig.spriteName.c_str());
            }
            else
            {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                    "Image dimensions: %dx%d pixels",
                    tex.extent.width, tex.extent.height);
            }#1#
        }*/

        ImGui::Separator();

        // Buttons
        ImGui::BeginGroup();
        if (ImGui::Button("Import", ImVec2(120, 0)))
        {
            if (spriteConfig.isSpriteSheet)
            {
                ImportAsSpriteSheet(
                    spriteConfig.targetPath,
                    spriteConfig.spriteCount,
                    spriteConfig.spriteName
                );
            }
            else
            {
                ImportAsSprite(spriteConfig.targetPath, spriteConfig.spriteName);
            }
            spriteConfig.showDialog = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)))
        {
            spriteConfig.showDialog = false;
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

void FileBrowserTab::ImportAsSpriteSheet(const std::filesystem::path& path, int spriteCount, const std::string& baseName)
{
    // Get path relative to root directory
    std::string relativePath{ CopyIntoWorkingDir(path).string() };

    // Load texture if not already loaded
    /*if (!ResourceManagerOld::TextureExists(relativePath))
    {
        ResourceManagerOld::LoadTexture(path.string(), relativePath);
    }*/

    /*const Texture& tex = ResourceManagerOld::GetTexture(relativePath);
    float spriteWidth = 1.0f / spriteCount;

    for (int i = 0; i < spriteCount; ++i)
    {
        Sprite sprite;
        sprite.name = baseName + "_" + std::to_string(i);
        sprite.textureName = relativePath;  // Store relative path
        sprite.textureID = tex.index;

        // Calculate UV coordinates for horizontal layout
        float u1 = i * spriteWidth;
        float u2 = (i + 1) * spriteWidth;
        sprite.texCoords = Vec4(u1, 0.0f, u2, 1.0f);

        // Calculate actual pixel dimensions
        sprite.width = tex.extent.width / spriteCount;
        sprite.height = tex.extent.height;

        ResourceManagerOld::AddSprite(sprite);
    }*/
}

void FileBrowserTab::ImportAsSprite(const std::filesystem::path& path, const std::string& name)
{
    // Get path relative to root directory
    std::string relativePath{ CopyIntoWorkingDir(path).string() };

    // Load texture if not already loaded
    /*if (!ResourceManagerOld::TextureExists(relativePath))
    {
        ResourceManagerOld::LoadTexture(path.string(), relativePath);
    }*/

    /*const Texture& tex = ResourceManagerOld::GetTexture(relativePath);

    Sprite sprite;
    sprite.name = name;
    sprite.textureName = relativePath;  // Store relative path
    sprite.textureID = tex.index;
    sprite.width = tex.extent.width;
    sprite.height = tex.extent.height;
    sprite.texCoords = Vec4(0.0f, 0.0f, 1.0f, 1.0f);

    ResourceManagerOld::AddSprite(sprite);*/
}

std::filesystem::path FileBrowserTab::CopyIntoWorkingDir(const std::filesystem::path& file)
{
    if (ST<Filepaths>::Get()->IsWithinWorkingDir(file))
        return ST<Filepaths>::Get()->MakeRelativeToWorkingDir(file);

    std::filesystem::path resultantFilepath{};
    switch (import::ImportToAssets(file, &resultantFilepath))
    {
    case import::IMPORT_RESULT::SUCCESS:
    case import::IMPORT_RESULT::ALREADY_IMPORTED:
        return ST<Filepaths>::Get()->MakeRelativeToWorkingDir(resultantFilepath);
    default:
        CONSOLE_LOG(LEVEL_ERROR) << "Failed to copy file into working directory: " << file.string();
        return file;
    }
}

