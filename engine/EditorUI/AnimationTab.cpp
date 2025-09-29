//#include "AnimationTab.h"
//#include "ResourceManager.h"
//#include "GUICollection.h"
//
//const char* AnimationTab::GetName() const
//{
//    return "Animations";
//}
//
//const char* AnimationTab::GetIdentifier() const
//{
//    return ICON_FA_FILM" Animations";
//}
//
//void AnimationTab::AnimationCreateConfig::Reset()
//{
//    showDialog = false;
//    animationName.clear();
//    frames.clear();
//    isPlaying = false;
//    previewTime = 0.0f;
//    currentFrame = 0;
//    spriteSearchBuffer[0] = '\0';
//    warningMessage.clear();
//    hasWarning = false;
//}
//
//void AnimationTab::Render()
//{
//#ifdef IMGUI_ENABLED
//    float THUMBNAIL_SIZE = AssetBrowser::THUMBNAIL_SIZE;
//
//    // Top control bar
//    if (ImGui::Button(ICON_FA_PLUS" Create Animation"))
//    {
//        animConfig.Reset();
//        animConfig.showDialog = true;
//    }
//
//    ImGui::Separator();
//
//    float panelWidth = ImGui::GetContentRegionAvail().x * 0.65f; // random offset
//    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
//    if (columnsCount < 1) columnsCount = 1;
//
//    // Push style variables for consistent spacing
//    // These will affect all ImGui elements until PopStyleVar
//    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
//
//    std::vector<size_t> animationsToDelete;
//    size_t itemCount = 0;
//    for (const auto& [nameHash, anim] : ResourceManagerOld::GetAnimations())
//    {
//        if (!ResourceManagerOld::ResourceExists(nameHash))
//        {
//            continue;
//        }
//        const std::string& animName = ResourceManagerOld::GetResourceName(nameHash);
//
//        if (!editor::MatchesFilter(animName)) continue;
//
//        ImGui::PushID(static_cast<int>(nameHash));
//
//        // Animation preview with button behavior
//        ImGui::BeginGroup();
//        const Sprite& sprite = ResourceManagerOld::GetSprite(anim.frames[0].spriteID);
//        //const Texture& tex = ResourceManagerOld::GetTexture(sprite.textureName);
//        if (!anim.frames.empty())
//        {
//            ImGui::ImageButton("##anim",
//                0,
//                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
//                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//                ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//        }
//        else
//        {
//            ImGui::Button("Empty Animation", ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE));
//        }
//
//        // Name label
//        std::string displayName = animName;
//        float maxWidth = THUMBNAIL_SIZE;
//        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
//        if (textSize.x > maxWidth)
//        {
//            while (textSize.x > maxWidth && displayName.length() > 3)
//            {
//                displayName = displayName.substr(0, displayName.length() - 4) + "...";
//                textSize = ImGui::CalcTextSize(displayName.c_str());
//            }
//        }
//        float textX = (THUMBNAIL_SIZE - textSize.x) * 0.5f;
//        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
//        ImGui::TextUnformatted(displayName.c_str());
//        ImGui::EndGroup();
//
//        // Hover tooltip
//        if (ImGui::IsItemHovered())
//        {
//            ImGui::BeginTooltip();
//            ImGui::Text("Name: %s", animName.c_str());
//            ImGui::Text("Frames: %zu", anim.frames.size());
//            float totalDuration = 0.0f;
//            for (const auto& frame : anim.frames)
//            {
//                totalDuration += frame.duration;
//            }
//            ImGui::Text("Duration: %.2fs", totalDuration);
//            ImGui::EndTooltip();
//        }
//
//        // Context menu with proper ID
//        if (ImGui::BeginPopupContextItem("animation_context"))
//        {
//            if (ImGui::MenuItem(ICON_FA_PENCIL" Edit"))
//            {
//                animConfig.Reset();
//                animConfig.showDialog = true;
//                animConfig.animationName = animName;
//                animConfig.frames = anim.frames;
//            }
//            if (ImGui::MenuItem(ICON_FA_CLONE" Duplicate"))
//            {
//                std::string newName = animName + "_copy";
//                ResourceManagerOld::CreateAnimationFromSprites(newName, anim.frames);
//            }
//            if (ImGui::MenuItem(ICON_FA_TRASH" Delete"))
//            {
//                animationsToDelete.push_back(nameHash);
//            }
//            ImGui::EndPopup();
//        }
//
//        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
//        {
//            // Set payload after ensuring we have a valid item ID
//            size_t i = nameHash;
//            ImGui::SetDragDropPayload("ANIM_HASH", &i, sizeof(size_t));
//
//            // Preview
//            ImGui::Image(
//                0,
//                ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
//                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//                ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//
//            ImGui::EndDragDropSource();
//        }
//
//        ImGui::PopID();
//
//        if (++itemCount % columnsCount != 0 && itemCount < ResourceManagerOld::GetAnimations().size())
//        {
//            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.x);
//        }
//    }
//
//    for (size_t nameHash : animationsToDelete)
//    {
//        ResourceManagerOld::DeleteAnimation(nameHash);  // Use proper deletion method
//    }
//
//
//    ImGui::PopStyleVar();
//
//    ShowCreateAnimationDialog();
//#endif
//}
//
//#ifdef IMGUI_ENABLED
//void AnimationTab::ShowCreateAnimationDialog()
//{
//    if (!animConfig.showDialog) return;
//
//    // Fixed size window, no resizing
//    ImGui::SetNextWindowSize(ImVec2(1200, 700));
//    if (ImGui::Begin("Create Animation", &animConfig.showDialog,
//        ImGuiWindowFlags_NoResize))
//    {
//
//        // Top bar with name input and search
//        ImGui::BeginChild("TopBar", ImVec2(0, 40), true);
//        {
//            // Name input
//            ImGui::AlignTextToFramePadding();
//            ImGui::Text("Name:"); ImGui::SameLine();
//            static char nameBuffer[256];
//            strncpy_s(nameBuffer, animConfig.animationName.c_str(), sizeof(nameBuffer) - 1);
//            ImGui::SetNextItemWidth(300);
//            if (ImGui::InputText("##name", nameBuffer, sizeof(nameBuffer)))
//            {
//                animConfig.animationName = nameBuffer;
//            }
//
//            // Search box (right-aligned)
//            ImGui::SameLine(ImGui::GetWindowWidth() - 320);
//            ImGui::SetNextItemWidth(300);
//            ImGui::InputTextWithHint("##search", ICON_FA_MAGNIFYING_GLASS" Search sprites...",
//                animConfig.spriteSearchBuffer,
//                sizeof(animConfig.spriteSearchBuffer));
//        }
//        ImGui::EndChild();
//
//        // Main content split with fixed columns
//        float contentHeight = ImGui::GetContentRegionAvail().y - 40;
//        ImGui::Columns(2, nullptr, true);
//        ImGui::SetColumnWidth(0, 800); // Give more space to sprite selection
//        ImGui::SetColumnWidth(1, 400); // Fixed width for frame list and preview
//
//        // Left side: Sprite selection
//        {
//            ImGui::BeginChild("Sprites", ImVec2(0, contentHeight));
//            RenderSpriteSelectionGrid();
//            ImGui::EndChild();
//        }
//
//        ImGui::NextColumn();
//
//        // Right side: Frame sequence and preview
//        {
//            ImGui::BeginChild("Frames", ImVec2(0, contentHeight));
//
//            // Frame list
//            ImGui::Text("Animation Frames (%zu):", animConfig.frames.size());
//            ImGui::BeginChild("FrameList", ImVec2(0, contentHeight * 0.5f), true);
//            RenderFrameList();  // Previously defined function
//            ImGui::EndChild();
//
//            // Preview section
//            ImGui::Text("Preview:");
//            ImGui::BeginChild("Preview", ImVec2(0, contentHeight * 0.4f), true);
//            RenderAnimationPreview();  // New separate function for preview
//            ImGui::EndChild();
//            ImGui::EndChild();
//        }
//
//        ImGui::Columns(1);
//
//        // Bottom bar with warnings and create/cancel buttons
//        RenderCreateAnimationBottom();
//    }
//    ImGui::End();
//}
//
//void AnimationTab::RenderSpriteSelectionGrid()
//{
//    float THUMBNAIL_SIZE = ST<AssetBrowser>::Get()->THUMBNAIL_SIZE;
//
//    // Calculate grid layout
//    // Calculate how many sprites can fit in a row based on available width
//    // ImGui::GetContentRegionAvail() returns the size of the current content area
//    float panelWidth = ImGui::GetContentRegionAvail().x;
//    int columnsCount = static_cast<int>(panelWidth / (THUMBNAIL_SIZE + 10));
//    if (columnsCount < 1) columnsCount = 1;
//
//    // Push style variables for consistent spacing
//    // These will affect all ImGui elements until PopStyleVar
//    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
//    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
//
//    size_t itemCount = 0;
//    for (size_t i = 0; i < ResourceManagerOld::GetSpriteCount(); i++)
//    {
//        const Sprite& sprite = ResourceManagerOld::GetSprite(i);
//        bool isValid = sprite.textureID != ResourceManagerOld::INVALID_TEXTURE_ID;
//        if (!isValid)
//            continue;
//        // Apply search filter (case-insensitive)
//        std::string searchStr = animConfig.spriteSearchBuffer;
//        std::string spriteName = sprite.name;
//        std::transform(searchStr.begin(), searchStr.end(), searchStr.begin(), util::ToLower);
//        std::transform(spriteName.begin(), spriteName.end(), spriteName.begin(), util::ToLower);
//
//        if (strlen(animConfig.spriteSearchBuffer) > 0 &&
//            spriteName.find(searchStr) == std::string::npos)
//        {
//            continue;
//        }
//
//        ImGui::PushID(static_cast<int>(i));
//
//        // Check if sprite is used in animation
//        bool isSelected = std::any_of(animConfig.frames.begin(),
//            animConfig.frames.end(),
//            [i](const FrameData& frame) {
//                return frame.spriteID == i;
//            });
//
//        // Calculate sprite usage count
//        size_t usageCount = std::count_if(animConfig.frames.begin(),
//            animConfig.frames.end(),
//            [i](const FrameData& frame) {
//                return frame.spriteID == i;
//            });
//
//        // Begin selectable group
//        ImGui::BeginGroup();
//
//        // Push colors for selection state
//        if (isSelected)
//        {
//            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.5f, 0.7f, 1.0f));
//            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.6f, 0.8f, 1.0f));
//            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.55f, 0.75f, 1.0f));
//        }
//
//        // Get sprite texture
//        //const Texture& tex = ResourceManagerOld::GetTexture(sprite.textureName);
//
//        // Pop selection colors if needed
//        if (isSelected)
//        {
//            ImGui::PopStyleColor(3);
//        }
//
//        // Handle click
//        if (ImGui::ImageButton(("sprite_" + std::to_string(i)).c_str(),  // Unique ID for each sprite button
//            0,
//            ImVec2(THUMBNAIL_SIZE, THUMBNAIL_SIZE),
//            ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//            ImVec2(sprite.texCoords.z, sprite.texCoords.w),
//            ImVec4(0, 0, 0, 0),
//            isSelected ? ImVec4(1, 1, 1, 1) : ImVec4(0.7f, 0.7f, 0.7f, 1)))
//        {
//            // Add new frame
//            FrameData newFrame;
//            newFrame.spriteID = i;
//            newFrame.duration = 0.5f;  // Default duration
//            animConfig.frames.push_back(newFrame);
//        }
//
//        // Show usage count if sprite is used multiple times
//        if (usageCount > 1)
//        {
//            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 0, 1));
//            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + THUMBNAIL_SIZE - 20);
//            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - THUMBNAIL_SIZE + 5);
//            ImGui::Text("x%zu", usageCount);
//            ImGui::PopStyleColor();
//            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + THUMBNAIL_SIZE - 20);//
//        }
//
//        // Sprite name (truncated if necessary)
//        std::string displayName = sprite.name;
//        ImVec2 textSize = ImGui::CalcTextSize(displayName.c_str());
//        if (textSize.x > THUMBNAIL_SIZE)
//        {
//            size_t maxLength = 10;
//            if (displayName.length() > maxLength)
//            {
//                displayName = displayName.substr(0, maxLength) + "...";
//            }
//        }
//
//        float textX = (THUMBNAIL_SIZE - ImGui::CalcTextSize(displayName.c_str()).x) * 0.5f;
//        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textX);
//        if (isSelected)
//        {
//            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 0.8f, 1.0f, 1.0f));
//        }
//        ImGui::TextUnformatted(displayName.c_str());
//        if (isSelected)
//        {
//            ImGui::PopStyleColor();
//        }
//
//        // Tooltip with full name and info
//        if (ImGui::IsItemHovered())
//        {
//            ImGui::BeginTooltip();
//            ImGui::Text("Name: %s", sprite.name.c_str());
//            ImGui::Text("Size: %dx%d", sprite.width, sprite.height);
//            if (usageCount > 0)
//            {
//                ImGui::Text("Used %zu times in animation", usageCount);
//            }
//            ImGui::EndTooltip();
//        }
//
//        ImGui::EndGroup();
//
//        // Drag and drop source
//        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
//        {
//            // Set payload after ensuring we have a valid item ID
//            size_t spriteId = i;
//            ImGui::SetDragDropPayload("SPRITE_ID", &spriteId, sizeof(size_t));
//
//            // Preview
//            ImGui::Image(0,
//                ImVec2(THUMBNAIL_SIZE / 2, THUMBNAIL_SIZE / 2),
//                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//                ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//
//            ImGui::EndDragDropSource();
//        }
//
//        ImGui::PopID();
//
//        // Grid layout
//        if (++itemCount % columnsCount != 0)
//        {
//            ImGui::SameLine();
//        }
//    }
//
//    ImGui::PopStyleVar(2);  // Pop ItemSpacing and FramePadding
//
//    // Drop target for removing sprites from selection
//    if (ImGui::BeginDragDropTarget())
//    {
//        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_SPRITE"))
//        {
//            size_t frameIndex = *(const size_t*)payload->Data;
//            if (frameIndex < animConfig.frames.size())
//            {
//                animConfig.frames.erase(animConfig.frames.begin() + frameIndex);
//            }
//        }
//        ImGui::EndDragDropTarget();
//    }
//}
//
//void AnimationTab::RenderFrameList()
//{
//    // Track deletions separately to avoid modifying collection while iterating
//    bool frameDeleted = false;
//    size_t frameToDelete = 0;
//
//    for (size_t i = 0; i < animConfig.frames.size(); i++)
//    {
//        // Push unique ID for this frame to avoid ID conflicts
//        ImGui::PushID(static_cast<int>(i));
//        const Sprite& sprite = ResourceManagerOld::GetSprite(animConfig.frames[i].spriteID);
//        //const Texture& tex = ResourceManagerOld::GetTexture(sprite.textureName);
//        // Frame Container
//         // Begin a group for the entire frame entry
//        ImGui::BeginGroup();
//        {
//            // Drag handle implementation
//           // Button acts as both visual handle and drag source
//            ImGui::Button("##draghandle", ImVec2(16, 40));
//
//            // Setup drag source
//            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
//            {
//
//                // Payload is the frame index
//                size_t frameIdx = i;
//                ImGui::SetDragDropPayload("FRAME_REORDER", &frameIdx, sizeof(size_t));
//                ImGui::Text("Moving frame %zu", i + 1);
//                // Show preview while dragging
//                ImGui::Image(0,
//                    ImVec2(20, 20),
//                    ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//                    ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//                ImGui::EndDragDropSource();
//            }
//
//            // Handle dropping frames for reordering
//            if (ImGui::BeginDragDropTarget())
//            {
//                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("FRAME_REORDER"))
//                {
//                    size_t srcIdx = *(const size_t*)payload->Data;
//                    if (srcIdx < animConfig.frames.size())
//                    {
//                        // ... frame reordering code ...
//                        FrameData temp = animConfig.frames[srcIdx];
//                        if (srcIdx < i)
//                        {
//                            std::rotate(animConfig.frames.begin() + srcIdx,
//                                animConfig.frames.begin() + srcIdx + 1,
//                                animConfig.frames.begin() + i + 1);
//                        }
//                        else
//                        {
//                            std::rotate(animConfig.frames.begin() + i,
//                                animConfig.frames.begin() + srcIdx,
//                                animConfig.frames.begin() + srcIdx + 1);
//                        }
//                        animConfig.frames[i] = temp;
//                    }
//                }
//                ImGui::EndDragDropTarget();
//            }
//            ImGui::SameLine();
//
//            // Frame number
//            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 12);
//            ImGui::Text("%zu.", i + 1);
//            ImGui::SameLine();
//
//            // Frame preview
//            ImGui::Image(0,
//                ImVec2(40, 40),
//                ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//                ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//            ImGui::SameLine();
//
//            // Frame info and controls
//            ImGui::BeginGroup();
//            {
//                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2);
//                ImGui::Text("%s", sprite.name.c_str());
//
//                ImGui::SetNextItemWidth(100);
//                float duration = animConfig.frames[i].duration;
//                if (ImGui::DragFloat("##duration", &duration, 0.01f, 0.01f, 5.0f, "%.2fs"))
//                {
//                    animConfig.frames[i].duration = duration;
//                }
//            }
//            ImGui::EndGroup();
//
//            // Right-side controls
//            float remainingWidth = ImGui::GetContentRegionAvail().x;
//            ImGui::SameLine(ImGui::GetCursorPosX() + remainingWidth - 90);
//            ImGui::BeginGroup();
//            {
//                ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10);
//
//                if (i > 0)
//                {
//                    if (ImGui::ArrowButton("##up", ImGuiDir_Up))
//                    {
//                        std::swap(animConfig.frames[i], animConfig.frames[i - 1]);
//                    }
//                }
//                else
//                {
//                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
//                    ImGui::ArrowButton("##up", ImGuiDir_Up);
//                    ImGui::PopStyleVar();
//                }
//                ImGui::SameLine();
//
//                if (i < animConfig.frames.size() - 1)
//                {
//                    if (ImGui::ArrowButton("##down", ImGuiDir_Down))
//                    {
//                        std::swap(animConfig.frames[i], animConfig.frames[i + 1]);
//                    }
//                }
//                else
//                {
//                    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.3f);
//                    ImGui::ArrowButton("##down", ImGuiDir_Down);
//                    ImGui::PopStyleVar();
//                }
//                ImGui::SameLine();
//
//                // Delete button - just mark for deletion instead of immediate removal
//                if (ImGui::Button(ICON_FA_TRASH"##delete"))
//                {
//                    frameDeleted = true;
//                    frameToDelete = i;
//                }
//            }
//            ImGui::EndGroup();
//        }
//        ImGui::EndGroup();
//
//        // Frame context menu
//        if (ImGui::BeginPopupContextItem(("frame_context_" + std::to_string(i)).c_str(),
//            ImGuiPopupFlags_MouseButtonRight))
//        {
//            if (ImGui::MenuItem("Remove Frame"))
//            {
//                frameDeleted = true;
//                frameToDelete = i;
//            }
//            if (i > 0 && ImGui::MenuItem("Move Up"))
//            {
//                std::swap(animConfig.frames[i], animConfig.frames[i - 1]);
//            }
//            if (i < animConfig.frames.size() - 1 && ImGui::MenuItem("Move Down"))
//            {
//                std::swap(animConfig.frames[i], animConfig.frames[i + 1]);
//            }
//            ImGui::EndPopup();
//        }
//
//        // Highlight current frame
//        if (i == animConfig.currentFrame && animConfig.isPlaying)
//        {
//            ImGui::GetWindowDrawList()->AddRect(
//                ImGui::GetItemRectMin(),
//                ImGui::GetItemRectMax(),
//                ImGui::GetColorU32(ImVec4(1, 1, 0, 1)),
//                2.0f,
//                ImDrawFlags_None,
//                2.0f
//            );
//        }
//
//        ImGui::PopID();
//
//        // Add separator between frames
//        if (i < animConfig.frames.size() - 1)
//        {
//            ImGui::Separator();
//        }
//    }
//
//    // Handle frame deletion after the loop
//    if (frameDeleted)
//    {
//        animConfig.frames.erase(animConfig.frames.begin() + frameToDelete);
//        // Adjust current frame if needed
//        if (animConfig.currentFrame >= animConfig.frames.size())
//        {
//            animConfig.currentFrame = animConfig.frames.empty() ? 0 : animConfig.frames.size() - 1;
//        }
//    }
//}
//
//void AnimationTab::RenderAnimationPreview()
//{
//    if (animConfig.frames.empty())
//    {
//        // Show helper text when no frames exist
//        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f),
//            "Add sprites to preview animation");
//        return;
//    }
//
//    // Update animation if playing
//    if (animConfig.isPlaying)
//    {
//        float currentTime = static_cast<float>(ImGui::GetTime());
//        // Calculate delta time, handling first frame case
//        float deltaTime = (lastFrameTime > 0.0f) ? (currentTime - lastFrameTime) : 0.0f;
//        lastFrameTime = currentTime;
//
//        if (deltaTime > 0.0f)
//        {
//            accumulatedTime += deltaTime;
//
//            // Get total duration
//            float totalDuration = 0.0f;
//            for (const auto& frame : animConfig.frames)
//            {
//                totalDuration += frame.duration;
//            }
//
//            // Handle loop
//            while (accumulatedTime >= totalDuration)
//            {
//                accumulatedTime -= totalDuration;
//            }
//
//            // Find current frame
//            float timeSum = 0.0f;
//            for (size_t i = 0; i < animConfig.frames.size(); ++i)
//            {
//                timeSum += animConfig.frames[i].duration;
//                if (accumulatedTime < timeSum)
//                {
//                    animConfig.currentFrame = i;
//                    break;
//                }
//            }
//        }
//    }
//
//    // Ensure current frame is valid
//    animConfig.currentFrame = std::min(animConfig.currentFrame, animConfig.frames.size() - 1);
//    const FrameData& currentFrame = animConfig.frames[animConfig.currentFrame];
//    const Sprite& sprite = ResourceManagerOld::GetSprite(currentFrame.spriteID);
//    //const Texture& tex = ResourceManagerOld::GetTexture(sprite.textureName);
//
//    // Center the preview
//    ImVec2 availSize = ImGui::GetContentRegionAvail();
//    availSize.y -= 40;  // Reserve space for controls
//
//    // Calculate scale maintaining aspect ratio
//    float scale = std::min(
//        availSize.y / sprite.height,
//        availSize.x / sprite.width
//    ) * 0.8f;
//
//    ImVec2 size(sprite.width * scale, sprite.height * scale);
//    ImVec2 pos(
//        (availSize.x - size.x) * 0.5f,
//        (availSize.y - size.y) * 0.5f
//    );
//
//    // Draw centered preview
//    ImGui::SetCursorPos(ImGui::GetCursorPos() + pos);
//    ImGui::Image(0,
//        size,
//        ImVec2(sprite.texCoords.x, sprite.texCoords.y),
//        ImVec2(sprite.texCoords.z, sprite.texCoords.w));
//
//    // Playback controls
//    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 35);
//    float controlsWidth = 250;
//    ImGui::SetCursorPosX((ImGui::GetWindowWidth() - controlsWidth) * 0.5f);
//
//    // Play/Pause button
//    if (ImGui::Button(animConfig.isPlaying ? ICON_FA_PAUSE : ICON_FA_PLAY))
//    {
//        animConfig.isPlaying = !animConfig.isPlaying;
//        if (animConfig.isPlaying)
//        {
//            lastFrameTime = static_cast<float>(ImGui::GetTime());  // Reset time when starting
//        }
//    }
//    ImGui::SameLine();
//
//    // Stop button
//    if (ImGui::Button(ICON_FA_STOP))
//    {
//        animConfig.isPlaying = false;
//        animConfig.currentFrame = 0;
//        accumulatedTime = 0.0f;
//        lastFrameTime = 0.0f;
//    }
//    ImGui::SameLine();
//
//    // Frame counter and duration
//    ImGui::Text("Frame %zu/%zu (%.2fs)",
//        animConfig.currentFrame + 1,
//        animConfig.frames.size(),
//        animConfig.frames[animConfig.currentFrame].duration);
//
//    // Progress bar
//    float totalDuration = 0.0f;
//    for (const auto& frame : animConfig.frames)
//    {
//        totalDuration += frame.duration;
//    }
//
//    float progress = totalDuration > 0.0f ? accumulatedTime / totalDuration : 0.0f;
//    ImGui::SetNextItemWidth(availSize.x * 0.8f);
//    if (ImGui::SliderFloat("##progress", &progress, 0.0f, 1.0f, ""))
//    {
//        // Update animation state based on slider
//        accumulatedTime = progress * totalDuration;
//        float timeSum = 0.0f;
//        for (size_t i = 0; i < animConfig.frames.size(); ++i)
//        {
//            timeSum += animConfig.frames[i].duration;
//            if (accumulatedTime < timeSum)
//            {
//                animConfig.currentFrame = i;
//                break;
//            }
//        }
//    }
//}
//
//void AnimationTab::RenderCreateAnimationBottom()
//{
//    ImGui::BeginChild("BottomBar", ImVec2(0, 30), true);
//
//    // Show warnings if any
//    if (animConfig.frames.empty())
//    {
//        animConfig.warningMessage = "Please add at least one frame";
//        animConfig.hasWarning = true;
//    }
//    else if (animConfig.animationName.empty())
//    {
//        animConfig.warningMessage = "Please enter an animation name";
//        animConfig.hasWarning = true;
//    }
//    else
//    {
//        animConfig.hasWarning = false;
//    }
//
//    if (animConfig.hasWarning)
//    {
//        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.4f, 0.4f, 1));
//        ImGui::Text("%s", animConfig.warningMessage.c_str());
//        ImGui::PopStyleColor();
//    }
//
//    // Create/Cancel buttons
//    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - 250);
//    bool canCreate = !animConfig.animationName.empty() && !animConfig.frames.empty();
//
//    if (ImGui::Button("Create", ImVec2(120, 0)) && canCreate)
//    {
//        ResourceManagerOld::CreateAnimationFromSprites(
//            animConfig.animationName,
//            animConfig.frames
//        );
//        animConfig.showDialog = false;
//    }
//    ImGui::SameLine();
//    if (ImGui::Button("Cancel", ImVec2(120, 0)))
//    {
//        animConfig.showDialog = false;
//    }
//
//    ImGui::EndChild();
//}
//#endif
