/******************************************************************************/
/*!
\file   AssetBrowser.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   27/11/2024

\author Ryan Cheong (95%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\author Chan Kuan Fu Ryan (5%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\brief
Asset browser for viewing and managing game assets.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "AssetBrowser.h"
#include "PrefabWindow.h"
#include "SceneManagement.h"
#include "EditorHistory.h"
#include "NameComponent.h"

#include "AudioManager.h"
#include "Filesystem.h"
#include "Import.h"

#include "CSScripting.h"
#include "ScriptManagement.h"
#include "HotReloader.h"
namespace fs = std::filesystem;

AssetBrowser::AssetBrowser() {
    // Initialize with default state
}

#ifdef IMGUI_ENABLED
void AssetBrowser::Draw(bool* p_open) {
    ImGui::SetNextWindowSize(ImVec2(1200, 600), ImGuiCond_FirstUseEver);

    if(!ImGui::Begin(ICON_FA_FOLDER" Browser", p_open)) {
        ImGui::End();
        return;
    }

    // Main layout
    RenderSidebar();
    ImGui::SameLine();

    RenderMainView();
    if(ImGui::BeginDragDropTarget())
    {
        // Accept the drag if the payload is of the correct type
        ImGuiPayload const* acceptedPayload = ImGui::AcceptDragDropPayload("ENTITY", ImGuiDragDropFlags_AcceptPeekOnly);
        if(acceptedPayload)
        {
            ecs::EntityHandle draggedEntity = *(ecs::EntityHandle*)acceptedPayload->Data;

            // Drop handling 
            currentCategory = CATEGORY::PREFABS;
            if(ImGui::IsMouseReleased(0))
            {
                PrefabManager::SavePrefab(draggedEntity, draggedEntity->GetComp<NameComponent>()->GetName());
            }
        }
        ImGui::EndDragDropTarget();
    }
    ShowSpriteSheetDialog();
    ShowCreateAnimationDialog();

    ImGui::End();
}
/**
 * @brief Renders the frame list with drag-drop reordering and frame controls
 *
 * Features:
 * - Drag and drop reordering of frames
 * - Per-frame duration control
 * - Frame deletion
 * - Frame movement controls (up/down)
 *
 * ImGui Patterns:
 * - BeginGroup/EndGroup for frame entries
 * - Drag and drop source/target
 * - Context menus
 * - Frame highlighting for current frame
 */
void AssetBrowser::RenderFrameList() {
    // Track deletions separately to avoid modifying collection while iterating
    bool frameDeleted = false;
    size_t frameToDelete = 0;

    for(size_t i = 0; i < animConfig.frames.size(); i++) {
        // Push unique ID for this frame to avoid ID conflicts
        ImGui::PushID(static_cast<int>(i));
        const Sprite& sprite = ResourceManager::GetSprite(animConfig.frames[i].spriteID);
        const Texture& tex = ResourceManager::GetTexture(sprite.textureName);
        // Frame Container
         // Begin a group for the entire frame entry
        ImGui::BeginGroup();
        {
            // Drag handle implementation
           // Button acts as both visual handle and drag source
            ImGui::Button("##draghandle", ImVec2(16, 40));

            // Setup drag source
            if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {

                // Payload is the frame index
                size_t frameIdx = i;
                ImGui::SetDragDropPayload("FRAME_REORDER", &frameIdx, sizeof(size_t));
                ImGui::Text("Moving frame %zu", i + 1);
                // Show preview while dragging
                ImGui::Image(tex.ImGui_handle,
                             ImVec2(20, 20),
                             ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                             ImVec2(sprite.texCoords.z, sprite.texCoords.w));
                ImGui::EndDragDropSource();
            }

            // Handle dropping frames for reordering
            if(ImGui::BeginDragDropTarget()) {
                if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_REORDER")) {
                    size_t srcIdx = *(const size_t*)payload->Data;
                    if(srcIdx < animConfig.frames.size()) {
                        // ... frame reordering code ...
                        FrameData temp = animConfig.frames[srcIdx];
                        if(srcIdx < i) {
                            std::rotate(animConfig.frames.begin() + srcIdx,
                                        animConfig.frames.begin() + srcIdx + 1,
                                        animConfig.frames.begin() + i + 1);
                        }
                        else {
                            std::rotate(animConfig.frames.begin() + i,
                                        animConfig.frames.begin() + srcIdx,
                                        animConfig.frames.begin() + srcIdx + 1);
                        }
                        animConfig.frames[i] = temp;
                    }
                }
                ImGui::EndDragDropTarget();
            }
            ImGui::SameLine();

            // Frame number
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12);
            ImGui::Text("%zu.", i + 1);
            ImGui::SameLine();

            // Frame preview
            ImGui::Image(tex.ImGui_handle,
                         ImVec2(40, 40),
                         ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                         ImVec2(sprite.texCoords.z, sprite.texCoords.w));
            ImGui::SameLine();

            // Frame info and controls
            ImGui::BeginGroup();
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
                ImGui::Text("%s", sprite.name.c_str());

                ImGui::SetNextItemWidth(100);
                float duration = animConfig.frames[i].duration;
                if(ImGui::DragFloat("##duration", &duration, 0.01f, 0.01f, 5.0f, "%.2fs")) {
                    animConfig.frames[i].duration = duration;
                }
            }
            ImGui::EndGroup();

            // Right-side controls
            float remainingWidth = ImGui::GetContentRegionAvail().x;
            ImGui::SameLine(ImGui::GetCursorPosX() + remainingWidth - 90);
            ImGui::BeginGroup();
            {
                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);

                if(i > 0) {
                    if(ImGui::ArrowButton("##up", ImGuiDir_Up)) {
                        std::swap(animConfig.frames[i], animConfig.frames[i - 1]);
                    }
                }
                else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
                    ImGui::ArrowButton("##up", ImGuiDir_Up);
                    ImGui::PopStyleVar();
                }
                ImGui::SameLine();

                if(i < animConfig.frames.size() - 1) {
                    if(ImGui::ArrowButton("##down", ImGuiDir_Down)) {
                        std::swap(animConfig.frames[i], animConfig.frames[i + 1]);
                    }
                }
                else {
                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
                    ImGui::ArrowButton("##down", ImGuiDir_Down);
                    ImGui::PopStyleVar();
                }
                ImGui::SameLine();

                // Delete button - just mark for deletion instead of immediate removal
                if(ImGui::Button(ICON_FA_TRASH"##delete")) {
                    frameDeleted = true;
                    frameToDelete = i;
                }
            }
            ImGui::EndGroup();
        }
        ImGui::EndGroup();

        // Frame context menu
        if(ImGui::BeginPopupContextItem(("frame_context_" + std::to_string(i)).c_str(),
                                        ImGuiPopupFlags_MouseButtonRight))
        {
            if(ImGui::MenuItem("Remove Frame")) {
                frameDeleted = true;
                frameToDelete = i;
            }
            if(i > 0 && ImGui::MenuItem("Move Up")) {
                std::swap(animConfig.frames[i], animConfig.frames[i - 1]);
            }
            if(i < animConfig.frames.size() - 1 && ImGui::MenuItem("Move Down")) {
                std::swap(animConfig.frames[i], animConfig.frames[i + 1]);
            }
            ImGui::EndPopup();
        }

        // Highlight current frame
        if(i == animConfig.currentFrame && animConfig.isPlaying) {
            ImGui::GetWindowDrawList()->AddRect(
                ImGui::GetItemRectMin(),
                ImGui::GetItemRectMax(),
                ImGui::GetColorU32(ImVec4(1, 1, 0, 1)),
                2.0f,
                ImDrawFlags_None,
                2.0f
            );
        }

        ImGui::PopID();

        // Add separator between frames
        if(i < animConfig.frames.size() - 1) {
            ImGui::Separator();
        }
    }

    // Handle frame deletion after the loop
    if(frameDeleted) {
        animConfig.frames.erase(animConfig.frames.begin() + frameToDelete);
        // Adjust current frame if needed
        if(animConfig.currentFrame >= animConfig.frames.size()) {
            animConfig.currentFrame = animConfig.frames.empty() ? 0 : animConfig.frames.size() - 1;
        }
    }
}
/**
 * @brief Renders the animation preview window with playback controls
 *
 * Layout:
 * - Centered preview image
 * - Bottom control bar with play/pause, stop, and progress
 * - Progress bar showing current position in animation
 *
 * State Management:
 * - Tracks playing state (isPlaying)
 * - Maintains accumulated time for frame timing
 * - Updates current frame based on frame durations
 */

void AssetBrowser::RenderAnimationPreview() {
    if(animConfig.frames.empty()) {
        // Show helper text when no frames exist
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                           "Add sprites to preview animation");
        return;
    }

    // Update animation if playing
    if(animConfig.isPlaying) {
        float currentTime = static_cast<float>(ImGui::GetTime());
        // Calculate delta time, handling first frame case
        float deltaTime = (lastFrameTime > 0.0f) ? (currentTime - lastFrameTime) : 0.0f;
        lastFrameTime = currentTime;

        if(deltaTime > 0.0f) {
            accumulatedTime += deltaTime;

            // Get total duration
            float totalDuration = 0.0f;
            for(const auto& frame : animConfig.frames) {
                totalDuration += frame.duration;
            }

            // Handle loop
            while(accumulatedTime >= totalDuration) {
                accumulatedTime -= totalDuration;
            }

            // Find current frame
            float timeSum = 0.0f;
            for(size_t i = 0; i < animConfig.frames.size(); ++i) {
                timeSum += animConfig.frames[i].duration;
                if(accumulatedTime < timeSum) {
                    animConfig.currentFrame = i;
                    break;
                }
            }
        }
    }

    // Ensure current frame is valid
    animConfig.currentFrame = std::min(animConfig.currentFrame, animConfig.frames.size() - 1);
    const FrameData& currentFrame = animConfig.frames[animConfig.currentFrame];
    const Sprite& sprite = ResourceManager::GetSprite(currentFrame.spriteID);
    const Texture& tex = ResourceManager::GetTexture(sprite.textureName);

    // Center the preview
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= 40;  // Reserve space for controls

    // Calculate scale maintaining aspect ratio
    float scale = std::min(
        availSize.y / sprite.height,
        availSize.x / sprite.width
    ) * 0.8f;

    ImVec2 size(sprite.width * scale, sprite.height * scale);
    ImVec2 pos(
        (availSize.x - size.x) * 0.5f,
        (availSize.y - size.y) * 0.5f
    );

    // Draw centered preview
    ImGui::SetCursorPos(ImGui::GetCursorPos() + pos);
    ImGui::Image(tex.ImGui_handle,
                 size,
                 ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                 ImVec2(sprite.texCoords.z, sprite.texCoords.w));

    // Playback controls
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 35);
    float controlsWidth = 250;
    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - controlsWidth) * 0.5f);

    // Play/Pause button
    if(ImGui::Button(animConfig.isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY)) {
        animConfig.isPlaying = !animConfig.isPlaying;
        if(animConfig.isPlaying) {
            lastFrameTime = static_cast<float>(ImGui::GetTime());  // Reset time when starting
        }
    }
    ImGui::SameLine();

    // Stop button
    if(ImGui::Button(ICON_FA_STOP)) {
        animConfig.isPlaying = false;
        animConfig.currentFrame = 0;
        accumulatedTime = 0.0f;
        lastFrameTime = 0.0f;
    }
    ImGui::SameLine();

    // Frame counter and duration
    ImGui::Text("Frame %zu/%zu (%.2fs)",
                animConfig.currentFrame + 1,
                animConfig.frames.size(),
                animConfig.frames[animConfig.currentFrame].duration);

    // Progress bar
    float totalDuration = 0.0f;
    for(const auto& frame : animConfig.frames) {
        totalDuration += frame.duration;
    }

    float progress = totalDuration > 0.0f ? accumulatedTime / totalDuration : 0.0f;
    ImGui::SetNextItemWidth(availSize.x * 0.8f);
    if(ImGui::SliderFloat("##progress", &progress, 0.0f, 1.0f, "")) {
        // Update animation state based on slider
        accumulatedTime = progress * totalDuration;
        float timeSum = 0.0f;
        for(size_t i = 0; i < animConfig.frames.size(); ++i) {
            timeSum += animConfig.frames[i].duration;
            if(accumulatedTime < timeSum) {
                animConfig.currentFrame = i;
                break;
            }
        }
    }
}

void AssetBrowser::RenderSidebar() {
    ImGui::BeginChild("Sidebar", ImVec2(SIDEBAR_WIDTH, 0), true);

    // Imported section
    if(ImGui::TreeNodeEx("Imported", ImGuiTreeNodeFlags_DefaultOpen)) {
        if(ImGui::Selectable(ICON_FA_IMAGE" Sprites", currentCategory == CATEGORY::SPRITES)) {
            currentCategory = CATEGORY::SPRITES;
        }
        if(ImGui::Selectable(ICON_FA_FILM" Animations", currentCategory == CATEGORY::ANIMATIONS)) {
            currentCategory = CATEGORY::ANIMATIONS;
        }
        if(ImGui::Selectable(ICON_FA_FONT" Fonts", currentCategory == CATEGORY::FONTS)) {
            currentCategory = CATEGORY::FONTS;
        }
        if(ImGui::Selectable(ICON_FA_MICROPHONE" Sounds", currentCategory == CATEGORY::SOUNDS)) {
            currentCategory = CATEGORY::SOUNDS;
        }
        if(ImGui::Selectable(ICON_FA_CUBE" Prefabs", currentCategory == CATEGORY::PREFABS)) {
            currentCategory = CATEGORY::PREFABS;
        }
        if (ImGui::Selectable(ICON_FA_DIAMOND" Scenes", currentCategory == CATEGORY::SCENES)) {
            currentCategory = CATEGORY::SCENES;
        }
        if(ImGui::Selectable(ICON_FA_CODE" Scripts", currentCategory == CATEGORY::SCRIPTS)) {
            currentCategory = CATEGORY::SCRIPTS;
        }
        if (ImGui::Selectable(ICON_FA_FILE_CODE" Shaders", currentCategory == CATEGORY::SHADERS)) {
            currentCategory = CATEGORY::SHADERS;
        }
        ImGui::TreePop();
    }

    // All Assets section with refresh button
    ImGui::BeginGroup();
    if(ImGui::Selectable("File Browser", currentCategory == CATEGORY::FILESYSTEM)) {
        currentCategory = CATEGORY::FILESYSTEM;
    }
    /*ImGui::SameLine();
    if(ImGui::Button(ICON_FA_ROTATE_RIGHT" ##refresh")) { // depreciated
        ResourceManager::ClearDirectoryCache();
    }*/
    ImGui::EndGroup();

    ImGui::EndChild();
}

void AssetBrowser::RenderMainView() {
    ImGui::BeginChild("MainView");

    // Toolbar with filter and breadcrumb
    RenderToolbar();

    // Content area
    switch (currentCategory)
    {
    case CATEGORY::FILESYSTEM:  RenderFileSystem();     break;
    case CATEGORY::SPRITES:     RenderSpriteGrid();     break;
    case CATEGORY::ANIMATIONS:  RenderAnimationGrid();  break;
    case CATEGORY::FONTS:       RenderFontWindow();     break;
    case CATEGORY::SOUNDS:      RenderSoundTable();     break;
    case CATEGORY::PREFABS:     RenderPrefabWindow();   break;
    case CATEGORY::SCENES:      RenderSceneWindow();    break;
    case CATEGORY::SCRIPTS:     RenderScriptWindow();   break;
    case CATEGORY::SHADERS:     RenderShadersWindow();  break;
    }

    ImGui::EndChild();
}

void AssetBrowser::RenderToolbar() {
    ImGui::BeginGroup();
    float windowWidth = ImGui::GetContentRegionAvail().x;
    float searchWidth = 300;
    float spacing = ImGui::GetStyle().ItemSpacing.x;

    if(currentCategory == CATEGORY::FILESYSTEM) {
        RenderNavigationBar();
        ImGui::SameLine();
    }
    // Get total available width
    // Location indicator with auto-truncation if needed
    std::string location = "Imported > ";
    switch(currentCategory) {
        case CATEGORY::FILESYSTEM:  location = "";  break;
        case CATEGORY::SPRITES:     location += "Sprites";      break;
        case CATEGORY::ANIMATIONS:  location += "Animations";   break;
        case CATEGORY::FONTS:       location += "Fonts";        break;
        case CATEGORY::SOUNDS:      location += "Sounds";       break;
        case CATEGORY::PREFABS:     location += "Prefabs";      break;
        case CATEGORY::SCENES:      location += "Scenes";       break;
        case CATEGORY::SCRIPTS:     location += "Scripts";      break;
        case CATEGORY::SHADERS:     location += "Shaders";      break;
        default: break;
    }


    if(location != "")
    {
        // Calculate maximum width for location text
        float maxLocationWidth = windowWidth - searchWidth - spacing * 2;
        ImVec2 textSize = ImGui::CalcTextSize(location.c_str());

        if(textSize.x > maxLocationWidth) {
            // Truncate text if needed
            while(textSize.x > maxLocationWidth && location.length() > 3) {
                location = location.substr(0, location.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(location.c_str());
            }
        }

        ImGui::Text("%s", location.c_str());
    }

    // Right-aligned search bar
    ImGui::SameLine(windowWidth - searchWidth);
    ImGui::SetNextItemWidth(searchWidth);
    ImGui::InputTextWithHint("##filter", ICON_FA_MAGNIFYING_GLASS" Search", searchBuffer, std::size(searchBuffer));

    ImGui::EndGroup();

}

void AssetBrowser::RenderFileSystem() {
    // Main file view
    ImGui::BeginChild("FileView", ImVec2(0, 0), true);
    {
        float panelWidth = ImGui::GetContentRegionAvail().x;
        int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
        if(columnsCount < 1) columnsCount = 1;

        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

        auto entries = file_system.ScanDirectory(file_system.GetCurrentPath());

        size_t itemCount = 0;
        for(const auto& entry : entries) {
            if(searchBuffer[0] != '\0' && !MatchesFilter(entry.filename)) {
                continue;
            }

            ImGui::BeginGroup();
            ImGui::PushID(entry.fullPath.string().c_str());

            bool clicked = false;

            // Render the item and check for right-click menu
            if(entry.isDirectory) {
                clicked = RenderDirectoryItem(entry);
                RenderItemLabel(entry.filename);
            }
            else {
                clicked = RenderFileItem(entry);
                if(ImGui::BeginPopupContextItem(entry.filename.c_str())) {
                    RenderItemContextMenu(entry);
                    ImGui::EndPopup();
                }
                if(ImGui::IsItemHovered()) {
                    ImGui::BeginTooltip();
                    ImGui::Text("Name: %s", entry.filename.c_str());
                    ImGui::Text("Type: %s", entry.fileType.c_str());
                    ImGui::Text("Path: %s", entry.fullPath.string().c_str());
                    ImGui::EndTooltip();
                }

                RenderItemLabel(entry.filename);
            }

            if(clicked && entry.isDirectory) {
                file_system.NavigateTo(entry.fullPath);
            }

            ImGui::PopID();
            ImGui::EndGroup();

            if((itemCount + 1) % columnsCount != 0 && itemCount < entries.size() - 1) {
                ImGui::SameLine();
            }
            itemCount++;
        }

        ImGui::PopStyleVar(2);
    }
    ImGui::EndChild();
}

void AssetBrowser::RenderNavigationBar() {
    // Back button
    if(ImGui::Button(ICON_FA_ARROW_LEFT) && file_system.CanNavigateBack()) {
        file_system.NavigateBack();
    }
    ImGui::SameLine();

    // Forward button
    if(ImGui::Button(ICON_FA_ARROW_RIGHT) && file_system.CanNavigateForward()) {
        file_system.NavigateForward();
    }
    ImGui::SameLine();

    // Up button
    if(ImGui::Button(ICON_FA_ARROW_UP)) {
        file_system.NavigateUp();
    }
    ImGui::SameLine();

    // Begin path bar group
    ImGui::BeginGroup();

    // Build path components from root to current
    std::vector<fs::path> pathComponents;
    fs::path currPath{ file_system.GetCurrentPath() };
    fs::path rootPath{ file_system.GetRootPath() };
    for (fs::path iterPath{ currPath }; iterPath != rootPath; iterPath = iterPath.parent_path())
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
            file_system.NavigateTo(*pathIter);

        gui::UnsetStyleColor popTextStyle{ isCurrentDir };

        if(ImGui::BeginPopupContextItem("path_context_menu")) {
            if(ImGui::MenuItem("Copy Path")) {
                ImGui::SetClipboardText(pathIter->string().c_str());
            }
            ImGui::EndPopup();
        }

        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("%s", pathIter->string().c_str());
            ImGui::EndTooltip();
        }
    }

    ImGui::EndGroup();
}

void AssetBrowser::AnimationCreateConfig::Reset() {
    showDialog = false;
    animationName.clear();
    frames.clear();
    isPlaying = false;
    previewTime = 0.0f;
    currentFrame = 0;
    spriteSearchBuffer[0] = '\0';
    warningMessage.clear();
    hasWarning = false;
}
/**
 * @brief Creates and manages the animation creation dialog
 *
 * Layout Structure:
 * - Fixed size window
 * - Top bar: Name input and search
 * - Two-column layout:
 *   Left: Sprite selection grid
 *   Right: Frame list and preview
 * - Bottom bar: Warnings and create/cancel buttons
 *
 * State Management:
 * - Tracks animation configuration in animConfig
 * - Manages frame selection and ordering
 * - Handles preview playback
 */
void AssetBrowser::ShowCreateAnimationDialog() {
    if(!animConfig.showDialog) return;

    // Fixed size window, no resizing
    ImGui::SetNextWindowSize(ImVec2(1200, 700));
    if(ImGui::Begin("Create Animation", &animConfig.showDialog,
                    ImGuiWindowFlags_NoResize)) {

        // Top bar with name input and search
        ImGui::BeginChild("TopBar", ImVec2(0, 40), true);
        {
            // Name input
            ImGui::AlignTextToFramePadding();
            ImGui::Text("Name:"); ImGui::SameLine();
            static char nameBuffer[256];
            strncpy_s(nameBuffer, animConfig.animationName.c_str(), sizeof(nameBuffer) - 1);
            ImGui::SetNextItemWidth(300);
            if(ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer))) {
                animConfig.animationName = nameBuffer;
            }

            // Search box (right-aligned)
            ImGui::SameLine(ImGui::GetWindowWidth() - 320);
            ImGui::SetNextItemWidth(300);
            ImGui::InputTextWithHint("##search", ICON_FA_MAGNIFYING_GLASS" Search sprites...",
                                     animConfig.spriteSearchBuffer,
                                     sizeof(animConfig.spriteSearchBuffer));
        }
        ImGui::EndChild();

        // Main content split with fixed columns
        float contentHeight = ImGui::GetContentRegionAvail().y - 40;
        ImGui::Columns(2, nullptr, true);
        ImGui::SetColumnWidth(0, 800); // Give more space to sprite selection
        ImGui::SetColumnWidth(1, 400); // Fixed width for frame list and preview

        // Left side: Sprite selection
        {
            ImGui::BeginChild("Sprites", ImVec2(0, contentHeight));
            RenderSpriteSelectionGrid();
            ImGui::EndChild();
        }

        ImGui::NextColumn();

        // Right side: Frame sequence and preview
        {
            ImGui::BeginChild("Frames", ImVec2(0, contentHeight));

            // Frame list
            ImGui::Text("Animation Frames (%zu):", animConfig.frames.size());
            ImGui::BeginChild("FrameList", ImVec2(0, contentHeight * 0.5f), true);
            RenderFrameList();  // Previously defined function
            ImGui::EndChild();

            // Preview section
            ImGui::Text("Preview:");
            ImGui::BeginChild("Preview", ImVec2(0, contentHeight * 0.4f), true);
            RenderAnimationPreview();  // New separate function for preview
            ImGui::EndChild();
            ImGui::EndChild();
        }

        ImGui::Columns(1);

        // Bottom bar with warnings and create/cancel buttons
        RenderCreateAnimationBottom();
    }
    ImGui::End();
}

/**
 * @brief Main render function for sprite selection grid in animation creator
 *
 * ImGui Layout Pattern Used:
 * - PushStyleVar for consistent spacing and padding
 * - Grid layout calculated from available width
 * - BeginGroup/EndGroup for treating multiple elements as one unit
 * - Image buttons with unique IDs for each sprite
 * - Tooltips for additional information
 * - Drag and drop functionality
 */
void AssetBrowser::RenderSpriteSelectionGrid() {
    // Calculate grid layout

    // Calculate how many sprites can fit in a row based on available width
    // ImGui::GetContentRegionAvail() returns the size of the current content area
    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
    if(columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

    size_t itemCount = 0;
    for(size_t i = 0; i < ResourceManager::GetSpriteCount(); i++) {
        const Sprite& sprite = ResourceManager::GetSprite(i);
        bool isValid = sprite.textureID != ResourceManager::INVALID_TEXTURE_ID;
        if (!isValid)
            continue;
        // Apply search filter (case-insensitive)
        std::string searchStr = animConfig.spriteSearchBuffer;
        std::string spriteName = sprite.name;
        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), util::ToLower);
        std::transform(spriteName.begin(), spriteName.end(), spriteName.begin(), util::ToLower);

        if(strlen(animConfig.spriteSearchBuffer) > 0 &&
           spriteName.find(searchStr) == std::string::npos) {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        // Check if sprite is used in animation
        bool isSelected = std::any_of(animConfig.frames.begin(),
                                      animConfig.frames.end(),
                                      [i](const FrameData& frame) {
            return frame.spriteID == i;
        });

        // Calculate sprite usage count
        size_t usageCount = std::count_if(animConfig.frames.begin(),
                                          animConfig.frames.end(),
                                          [i](const FrameData& frame) {
            return frame.spriteID == i;
        });

        // Begin selectable group
        ImGui::BeginGroup();

        // Push colors for selection state
        if(isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.7f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.8f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.55f, 0.75f, 1.0f));
        }

        // Get sprite texture
        const Texture& tex = ResourceManager::GetTexture(sprite.textureName);

        // Pop selection colors if needed
        if(isSelected) {
            ImGui::PopStyleColor(3);
        }

        // Handle click
        if(ImGui::ImageButton(("sprite_" + std::to_string(i)).c_str(),  // Unique ID for each sprite button
                              tex.ImGui_handle,
                              ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                              ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                              ImVec2(sprite.texCoords.z, sprite.texCoords.w),
                              ImVec4(0, 0, 0, 0),
                              isSelected ? ImVec4(1, 1, 1, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1)))
        {
            // Add new frame
            FrameData newFrame;
            newFrame.spriteID = i;
            newFrame.duration = 0.5f;  // Default duration
            animConfig.frames.push_back(newFrame);
        }

        // Show usage count if sprite is used multiple times
        if(usageCount > 1) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + THUMBNAIL_SIZE - 20);
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - THUMBNAIL_SIZE + 5);
            ImGui::Text("x%zu", usageCount);
            ImGui::PopStyleColor();
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + THUMBNAIL_SIZE - 20);//
        }

        // Sprite name (truncated if necessary)
        std::string displayName = sprite.name;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if(textSize.x > THUMBNAIL_SIZE) {
            size_t maxLength = 10;
            if(displayName.length() > maxLength) {
                displayName = displayName.substr(0, maxLength) + "...";
            }
        }

        float textX = (THUMBNAIL_SIZE - ImGui::CalcTextSize(displayName.c_str()).x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        if(isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
        }
        ImGui::TextUnformatted(displayName.c_str());
        if(isSelected) {
            ImGui::PopStyleColor();
        }

        // Tooltip with full name and info
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", sprite.name.c_str());
            ImGui::Text("Size: %dx%d", sprite.width, sprite.height);
            if(usageCount > 0) {
                ImGui::Text("Used %zu times in animation", usageCount);
            }
            ImGui::EndTooltip();
        }

        ImGui::EndGroup();

        // Drag and drop source
        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // Set payload after ensuring we have a valid item ID
            size_t spriteId = i;
            ImGui::SetDragDropPayload("SPRITE_ID", &spriteId, sizeof(size_t));

            // Preview
            ImGui::Image(tex.ImGui_handle,
                         ImVec2(THUMBNAIL_SIZE / 2, THUMBNAIL_SIZE / 2),
                         ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                         ImVec2(sprite.texCoords.z, sprite.texCoords.w));

            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        // Grid layout
        if(++itemCount % columnsCount != 0) {
            ImGui::SameLine();
        }
    }

    ImGui::PopStyleVar(2);  // Pop ItemSpacing and FramePadding

    // Drop target for removing sprites from selection
    if(ImGui::BeginDragDropTarget()) {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_SPRITE")) {
            size_t frameIndex = *(const size_t*)payload->Data;
            if(frameIndex < animConfig.frames.size()) {
                animConfig.frames.erase(animConfig.frames.begin() + frameIndex);
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void AssetBrowser::ShowSpriteSheetDialog() {
    if(!spriteConfig.showDialog) return;

    // Center the popup
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

    const char* title = spriteConfig.isSpriteSheet ? "Import Sprite Sheet" : "Import Sprite";
    ImGui::SetNextWindowSize(ImVec2(400.0f, spriteConfig.isSpriteSheet ? 250.0f : 150.0f), ImGuiCond_FirstUseEver);

    if(ImGui::Begin(title, &spriteConfig.showDialog,
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {

        // File information
        ImGui::TextWrapped("File: %s", spriteConfig.targetPath.filename().string().c_str());
        ImGui::Separator();

        // If it's a sprite sheet, show sprite count and assumptions
        if(spriteConfig.isSpriteSheet) {
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
        if(ImGui::InputText("Sprite Name", nameBuffer, sizeof(nameBuffer))) {
            spriteConfig.spriteName = nameBuffer;
        }

        std::string relativePath = ST<Filepaths>::Get()->MakeRelativeToWorkingDir(spriteConfig.targetPath);
        // Show dimensions info
        if(ResourceManager::TextureExists(relativePath)) {
            const Texture& tex = ResourceManager::GetTexture(relativePath);
            if(spriteConfig.isSpriteSheet) {
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
            else {
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
                                   "Image dimensions: %dx%d pixels",
                                   tex.extent.width, tex.extent.height);
            }
        }

        ImGui::Separator();

        // Buttons
        ImGui::BeginGroup();
        if(ImGui::Button("Import", ImVec2(120, 0))) {
            if(spriteConfig.isSpriteSheet) {
                ImportAsSpriteSheet(
                    spriteConfig.targetPath,
                    spriteConfig.spriteCount,
                    spriteConfig.spriteName
                );
            }
            else {
                ImportAsSprite(spriteConfig.targetPath, spriteConfig.spriteName);
            }
            spriteConfig.showDialog = false;
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel", ImVec2(120, 0))) {
            spriteConfig.showDialog = false;
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}

// TODO: This is a bit confusing with the relative to working directory or relative to exe...
// Probably should change ResourceManager::GetTexture() to use working directory to fix this.
void AssetBrowser::ImportAsSprite(const fs::path& path, const std::string& name) {
    // Get path relative to root directory
    std::string relativePath{ CopyIntoWorkingDir(path).string() };

    // Load texture if not already loaded
    if(!ResourceManager::TextureExists(relativePath)) {
        ResourceManager::LoadTexture(path.string(), relativePath);
    }

    const Texture& tex = ResourceManager::GetTexture(relativePath);

    Sprite sprite;
    sprite.name = name;
    sprite.textureName = relativePath;  // Store relative path
    sprite.textureID = tex.index;
    sprite.width = tex.extent.width;
    sprite.height = tex.extent.height;
    sprite.texCoords = Vector4(0.0f, 0.0f, 1.0f, 1.0f);

    ResourceManager::AddSprite(sprite);
}

void AssetBrowser::ImportAsSpriteSheet(const fs::path& path, int spriteCount, const std::string& baseName) {
    // Get path relative to root directory
    std::string relativePath{ CopyIntoWorkingDir(path).string() };

    // Load texture if not already loaded
    if(!ResourceManager::TextureExists(relativePath)) {
        ResourceManager::LoadTexture(path.string(), relativePath);
    }

    const Texture& tex = ResourceManager::GetTexture(relativePath);
    float spriteWidth = 1.0f / spriteCount;

    for(int i = 0; i < spriteCount; ++i) {
        Sprite sprite;
        sprite.name = baseName + "_" + std::to_string(i);
        sprite.textureName = relativePath;  // Store relative path
        sprite.textureID = tex.index;

        // Calculate UV coordinates for horizontal layout
        float u1 = i * spriteWidth;
        float u2 = (i + 1) * spriteWidth;
        sprite.texCoords = Vector4(u1, 0.0f, u2, 1.0f);

        // Calculate actual pixel dimensions
        sprite.width = tex.extent.width / spriteCount;
        sprite.height = tex.extent.height;

        ResourceManager::AddSprite(sprite);
    }
}

std::filesystem::path AssetBrowser::CopyIntoWorkingDir(const std::filesystem::path& file)
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

// Also update the LoadThumbnail function:
void AssetBrowser::LoadThumbnail(const fs::path& path) {
    std::string relativePath{ ST<Filepaths>::Get()->MakeRelativeToWorkingDir(path) };

    // First ensure the texture is loaded
    if(!ResourceManager::TextureExists(relativePath)) {
        ResourceManager::LoadTexture(path.string(), relativePath);
    }

    // Now we can safely get the texture and its handle
    const Texture& tex = ResourceManager::GetTexture(relativePath);
    thumbnailCache.textureDescriptors[path.string()] = tex.ImGui_handle;
}

VkDescriptorSet AssetBrowser::GetThumbnailDescriptor(const fs::path& path) {
    std::string pathStr = path.string();

    // Check if we already have the descriptor
    auto it = thumbnailCache.textureDescriptors.find(pathStr);
    if(it != thumbnailCache.textureDescriptors.end()) {
        return it->second;
    }

    // If we haven't tried loading it yet, try now
    if(!thumbnailCache.loadingStatus[pathStr]) {
        thumbnailCache.loadingStatus[pathStr] = true;

        // Attempt to load the thumbnail
        std::string relativePath = ST<Filepaths>::Get()->MakeRelativeToWorkingDir(path);
        if(!ResourceManager::TextureExists(relativePath)) {
            ResourceManager::LoadTexture(path.string(), relativePath);
        }

        // If texture was loaded successfully, cache its descriptor
        if(ResourceManager::TextureExists(relativePath)) {
            const Texture& tex = ResourceManager::GetTexture(relativePath);
            thumbnailCache.textureDescriptors[pathStr] = tex.ImGui_handle;
            return tex.ImGui_handle;
        }
    }
    return nullptr;
}
void AssetBrowser::DrawConfig()
{
    ImGui::Separator();
    ImGui::Text("Browser Settings");
    ImGui::DragFloat("Thumbnail Size", &THUMBNAIL_SIZE, 10.0f, 50.0f, 200.0f);
    ImGui::DragFloat("Sidebar Width", &SIDEBAR_WIDTH, 10.0f, 150.0f, 250.0f);
}

void AssetBrowser::RenderSpriteGrid() {
    float panelWidth = ImGui::GetContentRegionAvail().x * 0.65f; // random offset
    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
    if(columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
    size_t itemCount = 0;
    for(size_t i = 0; i < ResourceManager::GetSpriteCount(); ++i) {
        const Sprite& sprite = ResourceManager::GetSprite(i);
        bool isValid = sprite.textureID != ResourceManager::INVALID_TEXTURE_ID;
        if (!isValid)
            continue;

        if(!MatchesFilter(sprite.name)) {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));
        // Sprite preview with button behavior for context menu
        ImGui::BeginGroup();
        const Texture& tex = ResourceManager::GetTexture(sprite.textureName);
        ImGui::ImageButton("##sprite",
                            tex.ImGui_handle,
                            ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                            ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                            ImVec2(sprite.texCoords.z, sprite.texCoords.w));

        // Name label (dimmed if invalid)
        std::string displayName = sprite.name;
        float maxWidth = THUMBNAIL_SIZE;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if(textSize.x > maxWidth) {
            while(textSize.x > maxWidth && displayName.length() > 3) {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }

        float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::EndGroup();

        // Hover tooltip
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", sprite.name.c_str());
            ImGui::Text("Size: %dx%d", sprite.width, sprite.height);
            ImGui::Text("Texture: %s", sprite.textureName.c_str());

            // Add animation usage info to tooltip
            bool isUsed = false;
            int useCount = 0;
            for(const auto& [_, anim] : ResourceManager::GetAnimations()) {
                for(const auto& frame : anim.frames) {
                    if(frame.spriteID == i) {
                        isUsed = true;
                        useCount++;
                        break;
                    }
                }
            }
            if(isUsed) {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
                                    "Used in %d animation(s)", useCount);
            }

            ImGui::EndTooltip();
        }

        // Context menu with proper ID
        if(ImGui::BeginPopupContextItem("sprite_context")) {
            static char renameBuffer[256];
            static bool isRenaming = false;

            // Check if sprite is used in any animations
            bool isSpriteUsed = false;
            for(const auto& [_, anim] : ResourceManager::GetAnimations()) {
                if(std::any_of(anim.frames.begin(), anim.frames.end(),
                                [i](const FrameData& frame) { return frame.spriteID == i; })) {
                    isSpriteUsed = true;
                    break;
                }
            }

            if(isRenaming) {
                if(ImGui::InputText("##rename", renameBuffer, sizeof(renameBuffer),
                                    ImGuiInputTextFlags_EnterReturnsTrue)) {
                    ResourceManager::RenameSprite(i, renameBuffer);
                    isRenaming = false;
                }
                ImGui::SameLine();
                if(ImGui::Button("OK")) {
                    ResourceManager::RenameSprite(i, renameBuffer);
                    isRenaming = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if(ImGui::Button("Cancel")) {
                    isRenaming = false;
                    ImGui::CloseCurrentPopup();
                }
            }
            else {
                if(ImGui::MenuItem(ICON_FA_PEN" Rename")) {
                    strncpy_s(renameBuffer, sprite.name.c_str(), sizeof(renameBuffer) - 1);
                    isRenaming = true;
                }

                // Disable delete option if sprite is used in animations
                if(isSpriteUsed) {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                    ImGui::MenuItem(ICON_FA_TRASH" Delete (Used in Animations)", nullptr, false, false);
                    if(ImGui::IsItemHovered()) {
                        ImGui::SetTooltip("Cannot delete sprite while it's used in animations");
                    }
                    ImGui::PopStyleColor();
                }
                else {
                    if(ImGui::MenuItem(ICON_FA_TRASH" Delete")) {
                        ResourceManager::DeleteSprite(i);
                        ImGui::CloseCurrentPopup();
                    }
                }

                // Add info about sprite usage
                if(isSpriteUsed) {
                    ImGui::Separator();
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "Used in Animations:");
                    for(const auto& [nameHash, anim] : ResourceManager::GetAnimations()) {
                        bool usedInThisAnim = std::any_of(anim.frames.begin(), anim.frames.end(),
                                                            [i](const FrameData& frame) { return frame.spriteID == i; });
                        if(usedInThisAnim) {
                            ImGui::BulletText("%s", ResourceManager::GetResourceName(nameHash).c_str());
                        }
                    }
                }
            }
            ImGui::EndPopup();
        }

        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // Set payload after ensuring we have a valid item ID
            size_t spriteId = i;
            ImGui::SetDragDropPayload("SPRITE_ID", &spriteId, sizeof(size_t));

            // Preview
            const Texture& texture = ResourceManager::GetTexture(sprite.textureName);
            ImGui::Image(texture.ImGui_handle,
                            ImVec2(THUMBNAIL_SIZE / 2, THUMBNAIL_SIZE / 2),
                            ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                            ImVec2(sprite.texCoords.z, sprite.texCoords.w));

            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        if(++itemCount % columnsCount != 0 && itemCount < ResourceManager::GetSpriteCount()) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
    }

    ImGui::PopStyleVar();
}
void AssetBrowser::RenderPrefabWindow()
{
    float panelWidth = ImGui::GetContentRegionAvail().x * 0.65f; // random offset
    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
    if(columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
    std::string prefabName = "";
    for(size_t i = 0; i < PrefabManager::AllPrefabs().size(); ++i)
    {
        prefabName = PrefabManager::AllPrefabs()[i];
        if(!MatchesFilter(prefabName)) {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        ImGui::BeginGroup();
        bool clicked = ImGui::Button(ICON_FA_CUBE,
                                     ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        if (ImGui::IsItemHovered())
        {
            ImGui::BeginTooltip();
            ImGui::TextUnformatted(prefabName.c_str());
            ImGui::EndTooltip();
        }
        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            std::string sent_prefab = prefabName;
            ImGui::SetDragDropPayload("PREFAB", sent_prefab.c_str(), (sent_prefab.size() + 1) * sizeof(char));
            ImGui::Text(ICON_FA_CUBE);
            ImGui::EndDragDropSource();
        }
        if(clicked)
        {
            ecs::EntityHandle prefabEntity{ PrefabManager::LoadPrefab(prefabName) };
            ST<History>::Get()->OneEvent(HistoryEvent_EntityCreate{ prefabEntity });
        }

        // Name label
        std::string displayName = prefabName;
        float maxWidth = THUMBNAIL_SIZE;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if(textSize.x > maxWidth) {
            while(textSize.x > maxWidth && displayName.length() > 3) {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }
        float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::EndGroup();

        ImGui::PopID();
        if((static_cast<int>(i) + 1) % columnsCount != 0) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
        if (ImGui::BeginPopupContextItem(prefabName.c_str())) {
            if (ImGui::MenuItem(ICON_FA_TRASH" Delete")) {
                CONSOLE_LOG_EXPLICIT("DELETE " + prefabName,LogLevel::LEVEL_DEBUG);
                PrefabManager::DeletePrefab(prefabName);
            }
            if (ImGui::MenuItem(ICON_FA_CLONE" Duplicate")) {
                CONSOLE_LOG_EXPLICIT("CLONE " + prefabName,LogLevel::LEVEL_DEBUG);
            }
            ImGui::EndPopup();

        }
    }

    ImGui::PopStyleVar();
}

void AssetBrowser::RenderSceneWindow()
{
    float panelWidth = ImGui::GetContentRegionAvail().x * 0.65f; // random offset
    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
    if (columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

    int count{};
    for (const auto& entry : fs::directory_iterator{ ST<Filepaths>::Get()->scenesSave })
    {
        if (!MatchesFilter(entry.path().string()))
            continue;
        if (entry.path().extension() != ".scene")
            continue;

        ImGui::PushID(count++);

        ImGui::BeginGroup();

        if (ImGui::Button("##scene", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
        {
            ST<SceneManager>::Get()->LoadScene(entry.path());
        }

        // Name label
        std::string displayName = entry.path().stem().string();
        float maxWidth = THUMBNAIL_SIZE;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if (textSize.x > maxWidth) {
            while (textSize.x > maxWidth && displayName.length() > 3) {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }
        float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::EndGroup();

        ImGui::PopID();
        if ((count + 1) % columnsCount != 0) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
    }

    ImGui::PopStyleVar();
}

void AssetBrowser::RenderScriptWindow()
{
    static char buffer[1024] = "NewScript";
    ImGui::InputText(" ", buffer, sizeof(buffer));
    if (ImGui::Button("Create Script") && ST<ScriptManager>::Get()->EnsureScriptsFoldedrExists())
    {
        std::string name = buffer;
        name += ".cs";
        CONSOLE_LOG_EXPLICIT("Name of Script: " + name, LogLevel::LEVEL_DEBUG);

        if (!ST<ScriptManager>::Get()->CreateScript(name))
        {
            CONSOLE_LOG_EXPLICIT("Failed to create script:" + name, LogLevel::LEVEL_ERROR);

        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Reload"))
    {
        CSharpScripts::CSScripting::ReloadAssembly();
    }
    ImGui::Separator();

    std::string path = ST<Filepaths>::Get()->scriptsSave;
    std::vector<std::string> scriptFiles;

    if (ST<ScriptManager>::Get()->EnsureScriptsFoldedrExists())
    {
        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".cs")
            {
                scriptFiles.push_back(entry.path().filename().string());
            }
        }
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

    // Define the number of columns you want
    float thumbnailSize = THUMBNAIL_SIZE * 2;
    float availableWidth = ImGui::GetContentRegionAvail().x;
    int columnsCount = static_cast<int>(availableWidth / thumbnailSize);  // Calculate number of columns

    for (size_t i = 0; i < scriptFiles.size(); ++i)
    {
        const std::string& scriptname = scriptFiles[i];
        if (!MatchesFilter(scriptname))
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));

        ImGui::BeginGroup(); // Group to keep each button and label together

        // Create the thumbnail (button)
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // Set the drag-and-drop payload (here, the file name)
            std::string sent_script = scriptname;
            ImGui::SetDragDropPayload("SCRIPT", sent_script.c_str(), (sent_script.size() + 1) * sizeof(char));
            ImGui::Text(ICON_FA_CODE);  // Display the icon during dragging
            ImGui::EndDragDropSource();
        }

        // Draw the icon button (thumbnail)
        if (ImGui::Button(ICON_FA_CODE, ImVec2(thumbnailSize, thumbnailSize)))
        {
            // Single button click if needed
        }

        // Check for double-click on the item (icon)
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
        {
            // When double-clicked, load the script
            ST<ScriptManager>::Get()->OpenScript(scriptname);
        }

        // Script Name Label (showing truncated if it's too long)
        std::string displayName = scriptname;
        float maxWidth = thumbnailSize;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if (textSize.x > maxWidth)
        {
            // Truncate text if it's too wide
            while (textSize.x > maxWidth && displayName.length() > 3)
            {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }

        // Center the text label below the thumbnail
        float textX = (thumbnailSize - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());

        ImGui::EndGroup();

        ImGui::PopID();

        // Arrange items in columns
        if ((static_cast<int>(i) + 1) % columnsCount != 0)
        {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
        else
        {
            ImGui::NewLine();
        }
    }

    ImGui::PopStyleVar();
  
    //ImGui::PushID(count++);

    //ImGui::BeginGroup();

    //if (ImGui::Button("##scene", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE)))
    //{
    //    ST<SceneManager>::Get()->LoadScene(entry.path());
    //}

    //// Name label
    //std::string displayName = entry.path().stem().string();
    //float maxWidth = THUMBNAIL_SIZE;
    //ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
    //if (textSize.x > maxWidth) {
    //    while (textSize.x > maxWidth && displayName.length() > 3) {
    //        displayName = displayName.substr(0, displayName.length() - 4) + "...";
    //        textSize = ImGui::CalcTextSize(displayName.c_str());
    //    }
    //}
    //float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
    //ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
    //ImGui::TextUnformatted(displayName.c_str());
    //ImGui::EndGroup();

    //ImGui::PopID();
    
}

void AssetBrowser::RenderShadersWindow()
{
    static std::vector<std::string> shaderNames;
    if (shaderNames.empty())
    {
        for (const auto& entry : std::filesystem::directory_iterator{ ST<Filepaths>::Get()->shadersSave })
            shaderNames.push_back(entry.path().filename().string());
    }

    const float thumbnailSize{ 2 * THUMBNAIL_SIZE };

    float panelWidth = ImGui::GetContentRegionAvail().x;
    int columnsCount = static_cast<int>(panelWidth / (thumbnailSize + 10));
    if (columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    gui::SetStyleVar itemSpacingStyleVar{ gui::FLAG_STYLE_VAR::ITEM_SPACING, gui::Vec2{ 5, 5 } };

    int count{};
    for (const auto& shaderName : shaderNames)
    {
        gui::SetID id{ count++ };

        ImGui::BeginGroup();

        ImGui::Button("##shader", gui::Vec2{ thumbnailSize, thumbnailSize });

        // Name label
        std::string displayName = shaderName;
        float maxWidth = thumbnailSize;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if (textSize.x > maxWidth) {
            while (textSize.x > maxWidth && displayName.length() > 3) {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }
        float textX = (thumbnailSize - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::EndGroup();

        if ((count + 1) % columnsCount != 0) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
    }
}

void AssetBrowser::RenderFontWindow()
{
    const auto& fontAtlases = VulkanManager::Get().VkTextureManager().getFontAtlases();

    ImGui::BeginChild("FontTable", ImVec2(0.0f, 0.0f), true);
    ImGui::Text("Loaded Fonts");
    ImGui::Separator();

    for (const auto& [atlasName, atlas] : fontAtlases)
    {
        ImGui::Text("%s", atlasName.c_str());
    }

    ImGui::EndChild();
}

void AssetBrowser::RenderAnimationGrid() {
    // Top control bar
    if(ImGui::Button(ICON_FA_PLUS" Create Animation")) {
        animConfig.Reset();
        animConfig.showDialog = true;
    }

    ImGui::Separator();

    float panelWidth = ImGui::GetContentRegionAvail().x * 0.65f; // random offset
    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
    if(columnsCount < 1) columnsCount = 1;

    // Push style variables for consistent spacing
    // These will affect all ImGui elements until PopStyleVar
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));

    std::vector<size_t> animationsToDelete;
    size_t itemCount = 0;
    for(const auto& [nameHash, anim] : ResourceManager::GetAnimations()) {
        if (!ResourceManager::ResourceExists(nameHash)) {
            continue;
        }
        const std::string& animName = ResourceManager::GetResourceName(nameHash);

        if(!MatchesFilter(animName)) continue;

        ImGui::PushID(static_cast<int>(nameHash));

        // Animation preview with button behavior
        ImGui::BeginGroup();
        const Sprite& sprite = ResourceManager::GetSprite(anim.frames[0].spriteID);
        const Texture& tex = ResourceManager::GetTexture(sprite.textureName);
        if(!anim.frames.empty()) {
            ImGui::ImageButton("##anim",
                               tex.ImGui_handle,
                               ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                               ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                               ImVec2(sprite.texCoords.z, sprite.texCoords.w));
        }
        else {
            ImGui::Button("Empty Animation", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        }

        // Name label
        std::string displayName = animName;
        float maxWidth = THUMBNAIL_SIZE;
        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
        if(textSize.x > maxWidth) {
            while(textSize.x > maxWidth && displayName.length() > 3) {
                displayName = displayName.substr(0, displayName.length() - 4) + "...";
                textSize = ImGui::CalcTextSize(displayName.c_str());
            }
        }
        float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
        ImGui::TextUnformatted(displayName.c_str());
        ImGui::EndGroup();

        // Hover tooltip
        if(ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Name: %s", animName.c_str());
            ImGui::Text("Frames: %zu", anim.frames.size());
            float totalDuration = 0.0f;
            for(const auto& frame : anim.frames) {
                totalDuration += frame.duration;
            }
            ImGui::Text("Duration: %.2fs", totalDuration);
            ImGui::EndTooltip();
        }

        // Context menu with proper ID
        if(ImGui::BeginPopupContextItem("animation_context")) {
            if(ImGui::MenuItem(ICON_FA_PENCIL" Edit")) {
                animConfig.Reset();
                animConfig.showDialog = true;
                animConfig.animationName = animName;
                animConfig.frames = anim.frames;
            }
            if(ImGui::MenuItem(ICON_FA_CLONE" Duplicate")) {
                std::string newName = animName + "_copy";
                ResourceManager::CreateAnimationFromSprites(newName, anim.frames);
            }
        if(ImGui::MenuItem(ICON_FA_TRASH" Delete")) {
            animationsToDelete.push_back(nameHash);
            }
            ImGui::EndPopup();
        }

        if(ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
        {
            // Set payload after ensuring we have a valid item ID
            size_t i = nameHash;
            ImGui::SetDragDropPayload("ANIM_HASH", &i, sizeof(size_t));

            // Preview
            ImGui::Image(
                tex.ImGui_handle,
                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
                ImVec2(sprite.texCoords.z, sprite.texCoords.w));

            ImGui::EndDragDropSource();
        }

        ImGui::PopID();

        if(++itemCount % columnsCount != 0 && itemCount < ResourceManager::GetAnimations().size()) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
        }
    }

    for(size_t nameHash : animationsToDelete) {
        ResourceManager::DeleteAnimation(nameHash);  // Use proper deletion method
    }


    ImGui::PopStyleVar();
}

void AssetBrowser::RenderSoundTable()
{
    ImGui::BeginChild("SoundTable", ImVec2(0.0f, 0.0f), true);

    std::set<std::string> singleSoundNames = ST<AudioManager>::Get()->GetSingleSoundNames();
    std::map<std::string, std::set<std::string>> groupedSoundNames = ST<AudioManager>::Get()->GetGroupedSoundNames();

    ImGui::Columns(2, nullptr, true);

    // Left column for single sounds
    ImGui::Text("Single Sounds");

    // Iterate all single sound names
    for (std::string const& name : singleSoundNames)
    {
        // Create Button
        if (ImGui::Selectable(name.c_str()))
        {
            if (ST<AudioManager>::Get()->IsSoundPlaying(name))
            {
                ST<AudioManager>::Get()->StopSound(name);
            }
            else
            {
                ST<AudioManager>::Get()->StartSingleSound(name, false);
            }
        }

        // Drag-drop source
        gui::PayloadSource("SOUND", name, name.c_str());

        // Context menu
        RenderSoundContextMenu(name, false);
    }

    // Right column for grouped sounds
    ImGui::NextColumn();
    ImGui::Text("Grouped Sounds");
    ImGui::Separator();

    // Iterate all grouped sound names
    for (std::pair<std::string, std::set<std::string>> const& group : groupedSoundNames)
    {
        if (ImGui::TreeNode(group.first.c_str()))
        {
            ImGui::Indent(55.0f);
            for (std::string const& name : group.second)
            {
                // Create Button
                if (ImGui::Selectable(name.c_str()))
                {
                    if (ST<AudioManager>::Get()->IsSoundPlaying(name))
                    {
                        ST<AudioManager>::Get()->StopSound(name);
                    }
                    else
                    {
                        ST<AudioManager>::Get()->StartSpecificGroupedSound(name, false);
                    }
                }

                // Single sound drag-drop source
                gui::PayloadSource("SOUND", name, name.c_str());

                // Context menu
                RenderSoundContextMenu(name, true);
            }
            ImGui::Unindent(55.0f);
            ImGui::TreePop();
        }

        // Grouped sound drag-drop source
        gui::PayloadSource("SOUND", group.first, group.first.c_str());
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void AssetBrowser::RenderSoundContextMenu(std::string const& name, bool isGrouped)
{
    if (ImGui::BeginPopupContextItem(("Delete##" + name).c_str()))
    {
        if (ImGui::MenuItem("Delete"))
        {
            ST<AudioManager>::Get()->DeleteSound(name, isGrouped);
        }
        ImGui::EndPopup();
    }
}

void AssetBrowser::RenderCreateAnimationBottom() {
    ImGui::BeginChild("BottomBar", ImVec2(0, 30), true);

    // Show warnings if any
    if(animConfig.frames.empty()) {
        animConfig.warningMessage = "Please add at least one frame";
        animConfig.hasWarning = true;
    }
    else if(animConfig.animationName.empty()) {
        animConfig.warningMessage = "Please enter an animation name";
        animConfig.hasWarning = true;
    }
    else {
        animConfig.hasWarning = false;
    }

    if(animConfig.hasWarning) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4f, 0.4f, 1));
        ImGui::Text("%s", animConfig.warningMessage.c_str());
        ImGui::PopStyleColor();
    }

    // Create/Cancel buttons
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);
    bool canCreate = !animConfig.animationName.empty() && !animConfig.frames.empty();

    if(ImGui::Button("Create", ImVec2(120, 0)) && canCreate) {
        ResourceManager::CreateAnimationFromSprites(
            animConfig.animationName,
            animConfig.frames
        );
        animConfig.showDialog = false;
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel", ImVec2(120, 0))) {
        animConfig.showDialog = false;
    }

    ImGui::EndChild();
}

bool AssetBrowser::RenderDirectoryItem(const FileSystem::FileEntry& entry) {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    bool clicked = ImGui::Button((ICON_FA_FOLDER"##" + entry.filename).c_str(),
                                 ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    ImGui::PopStyleColor();
    return clicked;
}

bool AssetBrowser::RenderFileItem(const FileSystem::FileEntry& entry) {
    bool clicked;

    if(entry.fileType == "texture") {
        VkDescriptorSet descriptor = GetThumbnailDescriptor(entry.fullPath);
        if(descriptor) {
            clicked = ImGui::ImageButton(("##" + entry.filename).c_str(),
                                         descriptor,
                                         ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        }
        else {
            clicked = ImGui::Button((ICON_FA_IMAGE"##" + entry.filename).c_str(),
                                    ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
        }
    }
    else if(entry.fileType == "audio") {
        clicked = ImGui::Button((ICON_FA_MUSIC"##" + entry.filename).c_str(),
                                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    else if(entry.fileType == "font") {
        clicked = ImGui::Button((ICON_FA_FONT"##" + entry.filename).c_str(),
                                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    else {
        clicked = ImGui::Button((ICON_FA_FILE"##" + entry.filename).c_str(),
                                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
    }
    return clicked;
}

void AssetBrowser::RenderItemLabel(const std::string& filename) {
    std::string displayName = filename;
    ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());

    if(textSize.x > THUMBNAIL_SIZE) {
        while(textSize.x > THUMBNAIL_SIZE && displayName.length() > 3) {
            displayName = displayName.substr(0, displayName.length() - 4) + "...";
            textSize = ImGui::CalcTextSize(displayName.c_str());
        }
    }

    float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
    ImGui::TextUnformatted(displayName.c_str());
}

void AssetBrowser::RenderItemContextMenu(const FileSystem::FileEntry& entry) {

    if(entry.fileType == "texture") {
        if(ImGui::MenuItem(ICON_FA_FILE_IMPORT" Import as Sprite")) {
            spriteConfig.showDialog = true;
            spriteConfig.targetPath = entry.fullPath;
            spriteConfig.spriteName = entry.fullPath.stem().string();
            spriteConfig.isSpriteSheet = false;
        }
        if(ImGui::MenuItem(ICON_FA_FILE_IMPORT" Import as Sprite Sheet")) {
            spriteConfig.showDialog = true;
            spriteConfig.targetPath = entry.fullPath;
            spriteConfig.spriteName = entry.fullPath.stem().string();
            spriteConfig.isSpriteSheet = true;
            spriteConfig.spriteCount = 1;
        }
        ImGui::Separator();
    }

    if(ImGui::MenuItem(ICON_FA_COPY" Copy Path")) {
        ImGui::SetClipboardText(entry.fullPath.string().c_str());
    }
    if(ImGui::MenuItem(ICON_FA_FOLDER_OPEN" Show in Explorer")) {
        std::string command = "explorer /select,\"" + entry.fullPath.string() + "\"";
        system(command.c_str());
    }
}

bool AssetBrowser::MatchesFilter(const std::string& name) const {
    std::string lowerName = name;
    std::string lowerFilter = searchBuffer;
    std::ranges::transform(lowerName, lowerName.begin(), util::ToLower);
    std::ranges::transform(lowerFilter, lowerFilter.begin(), util::ToLower);
    return lowerName.find(lowerFilter) != std::string::npos;
}
#endif