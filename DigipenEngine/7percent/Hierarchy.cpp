/******************************************************************************/
/*!
\file   Hierarchy.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   02/11/2024

\author Chan Kuan Fu Ryan (80%)
\par    email: c.kuanfuryan\@digipen.edu
\par    DigiPen login: c.kuanfuryan

\author Kendrick Sim Hean Guan (20%)
\par    email: kendrickheanguan.s\@digipen.edu
\par    DigiPen login: kendrickheanguan.s

\brief
	Handles the rendering and logic of Hierarchy window through ImGui.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include "Hierarchy.h"
#include "ResourceManager.h"
#include "SceneManagement.h"
#include "Editor.h"
#include "TweenManager.h"
#include "NameComponent.h"
#ifdef IMGUI_ENABLED

Hierarchy::Hierarchy()
	: lastClickedEntity{ nullptr }
	, entityToRename{ nullptr }
	, isRenamingEntity{ false }, sceneIndexToRename{ -1 }
	, isRenamingScene{ false }
{
}

void Hierarchy::Draw()
{
    // ImGui window with custom styling for better visual hierarchy
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.4f, 0.6f, 0.6f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.5f, 0.7f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.35f, 0.55f, 0.75f, 0.8f));

    // Remove NoScrollbar flag to allow the main window to have a scrollbar
    ImGui::Begin(ICON_FA_SITEMAP" Hierarchy", &isOpen);

    // Store available window height for scene layout calculations
    availableWindowHeight = ImGui::GetContentRegionAvail().y;
    usedHeight = 0.0f;
    
    // Count total number of scenes for layout calculations
    int sceneCount = 0;
    for (auto sceneIter{ ST<SceneManager>::Get()->GetScenesBegin() }, 
         endIter{ ST<SceneManager>::Get()->GetScenesEnd() }; 
         sceneIter != endIter; ++sceneIter) {
        sceneCount++;
    }

    CheckBackgroundWindowContextMenu();
    // Track how much height is used by header elements
    usedHeight += ImGui::GetItemRectSize().y + ImGui::GetStyle().ItemSpacing.y;

    // Search field row with clear button
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8, 4));
    
    static gui::TextBoxWithFilter textInput(false);
    
    // Calculate widths for search components
    float availWidth = ImGui::GetContentRegionAvail().x;
    float searchIconWidth = ImGui::CalcTextSize(ICON_FA_MAGNIFYING_GLASS).x + ImGui::GetStyle().ItemSpacing.x;
    float clearButtonWidth = ImGui::CalcTextSize(ICON_FA_TRASH).x + 16.0f;
    float searchFieldWidth = availWidth - searchIconWidth - clearButtonWidth - ImGui::GetStyle().ItemSpacing.x;
    
    // Search icon
    ImGui::Text(ICON_FA_MAGNIFYING_GLASS);
    ImGui::SameLine();
    
    // Search field
    gui::SetNextItemWidth(searchFieldWidth);
    textInput.Draw("##Search");
    
    // Clear button
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8f, 0.3f, 0.3f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9f, 0.4f, 0.4f, 0.8f));
    if (ImGui::Button(ICON_FA_TRASH, ImVec2(clearButtonWidth, 0)) && !textInput.IsEmpty())
    {
        textInput.Clear();
    }
    if(ImGui::IsItemHovered())
        ImGui::SetTooltip("Clear Filter");
    ImGui::PopStyleColor(2); // Pop clear button colors
    
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor();
    ImGui::Spacing();
    // Track search bar height
    usedHeight += ImGui::GetItemRectSize().y + ImGui::GetStyle().ItemSpacing.y;

      // "New Scene" button
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.6f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.55f, 0.7f, 0.8f));
    if (ImGui::Button(ICON_FA_PLUS " New Scene", ImVec2(ImGui::GetContentRegionAvail().x, 0)))
    {
        ST<SceneManager>::Get()->CreateEmptyScene("New Scene");
    }
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
    ImGui::Spacing();

    // Styled separator
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::Separator();
    ImGui::PopStyleColor();
    
    usedHeight += ImGui::GetStyle().ItemSpacing.y;
    
    // Calculate remaining height for scenes
    float remainingHeight = availableWindowHeight - usedHeight;

    // Display each loaded scene with enhanced visuals
    for (auto sceneIter{ ST<SceneManager>::Get()->GetScenesBegin() }, 
         endIter{ ST<SceneManager>::Get()->GetScenesEnd() }; 
         sceneIter != endIter; ++sceneIter)
    {
        int sceneIndex{ sceneIter->GetIndex() };
        bool isActiveScene = ST<SceneManager>::Get()->GetActiveScene()->GetIndex() == sceneIndex;

        // Visual indication for active scene
        if (isActiveScene) {
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.2f, 0.6f, 0.3f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.3f, 0.7f, 0.4f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.35f, 0.75f, 0.45f, 0.8f));
        }

        // Start the tree object
        bool isTreeOpen;
        if (sceneIndexToRename == sceneIndex)
        {
            HandleSceneRenaming();
            if (isActiveScene) ImGui::PopStyleColor(3);
            continue;
        }
        else
        {
            ImGuiTreeNodeFlags treeNodeFlags{
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth |
                 (isActiveScene ? ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Selected : 0) |
                  (expandedSceneIndices.contains(sceneIndex) ? ImGuiTreeNodeFlags_DefaultOpen : 0)
            };

            // Scene name with icon and count
            int entityCount = 0;
            for (auto entityIter = sceneIter->GetEntitiesBegin(); entityIter != sceneIter->GetEntitiesEnd(); ++entityIter) {
                entityCount++;
            }

            std::string sceneLabel = std::string(ICON_FA_FILM) + " " + sceneIter->GetName() + " (" + std::to_string(entityCount) + ")";

            isTreeOpen = ImGui::TreeNodeEx(sceneLabel.c_str(), treeNodeFlags);
        }

        if (isActiveScene) ImGui::PopStyleColor(3);

        // Check for context menu on the tree object
        if (CheckSceneContextMenu(sceneIndex))
        {
            // A scene was removed. This invalidates our iterator. We'll need to stop rendering the rest of this frame.
            if (isTreeOpen)
                ImGui::TreePop();
            break;
        }

        // Now we can start rendering the entity items
        if (isTreeOpen)
        {
          expandedSceneIndices.insert(sceneIndex);
          // Add entity button
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.4f, 0.5f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.2f, 0.5f, 0.6f, 0.8f));
            if (ImGui::Button(ICON_FA_PLUS " Add Entity", ImVec2(ImGui::GetContentRegionAvail().x - 10, 0)))
            {
                // Set active scene first
                ST<SceneManager>::Get()->SetActiveScene(sceneIndex);
                ST<Editor>::Get()->CreateEntityAndSelect();
            }
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar();
            ImGui::Spacing();

            // Create a visual drop area instead of a child window
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.15f, 0.17f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_Border, isActiveScene ?
                ImVec4(0.2f, 0.5f, 0.3f, 0.7f) :
                ImVec4(0.3f, 0.4f, 0.5f, 0.5f));
            ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 4.0f);

            // Get and sort all top-level entities from this scene
            std::vector<ecs::EntityHandle> topLevelEntities;
            for (auto entityIter = sceneIter->GetEntitiesBegin(); entityIter != sceneIter->GetEntitiesEnd(); ++entityIter)
            {
                if (!entityIter->GetTransform().GetParent())
                {
                    topLevelEntities.push_back(*entityIter);
                }
            }

            // Sort entities by name for consistent ordering
            std::sort(topLevelEntities.begin(), topLevelEntities.end(),
                [](const ecs::EntityHandle& a, const ecs::EntityHandle& b) {
                    return a->GetComp<NameComponent>()->GetName() < b->GetComp<NameComponent>()->GetName();
                });
                
            // Calculate a dynamic height based on number of entities
            // Base height for the container with no entities
            float baseHeight = 40.0f; // Enough for "No entities" message
            
            // Estimate height for each entity (including potential children)
            // This is an approximation - you may need to adjust these values
            float estimatedEntityHeight = 24.0f; // Basic height for a tree node
            float maxEntitiesHeight = topLevelEntities.size() * estimatedEntityHeight;
            
            // Add space for the Add Entity button
            float totalEstimatedHeight = baseHeight + maxEntitiesHeight + 30.0f;
            
            // Cap the maximum height to ensure other scenes are still accessible
            // but allow it to grow with the number of entities
            float maxExpandedHeight = std::min(totalEstimatedHeight, remainingHeight * 0.7f);
            
            // Ensure minimum height for drop target text visibility
            float minDropTargetHeight = ImGui::CalcTextSize("Drop here to move to this scene").y + 30.0f;
            maxExpandedHeight = std::max(maxExpandedHeight, minDropTargetHeight);
            
            ImGui::BeginChild(("SceneDropTarget_" + std::to_string(sceneIndex)).c_str(),
                ImVec2(ImGui::GetContentRegionAvail().x, maxExpandedHeight), 
                true, ImGuiWindowFlags_AlwaysVerticalScrollbar); // Always show scrollbar when needed

            // We've already collected and sorted the entities above

            // Display all sorted top-level entities
            bool hasEntities = !topLevelEntities.empty();
            for (auto& entity : topLevelEntities)
            {
                ShowEntity(entity, sceneIndex, textInput);
            }

            // Show empty state message if no entities
            if (!hasEntities && textInput.IsEmpty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
                ImGui::Spacing();
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("No entities in this scene").x) * 0.5f);
                ImGui::TextUnformatted("No entities in this scene");
                ImGui::Spacing();
                ImGui::PopStyleColor();
            }
            else if (!hasEntities && !textInput.IsEmpty()) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
                ImGui::Spacing();
                ImGui::SetCursorPosX((ImGui::GetWindowWidth() - ImGui::CalcTextSize("No matching entities found").x) * 0.5f);
                ImGui::TextUnformatted("No matching entities found");
                ImGui::Spacing();
                ImGui::PopStyleColor();
            }

            ImGui::EndChild();
            ImGui::PopStyleVar(2);
            ImGui::PopStyleColor(2);

            // Handle unparenting as a drop target for the entire scene area
            HandleSceneDropTarget(sceneIndex);

            ImGui::TreePop();
        }
      else
      {
        expandedSceneIndices.erase(sceneIndex);
      }
    }

    ImGui::End();

    // Pop style settings
    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();
}

bool Hierarchy::ShowEntity(ecs::EntityHandle entity, int targetSceneIndex, const gui::TextBoxWithFilter& textFilter)
{
    std::string id                         { std::to_string(entity->GetHash()) };
    std::string name                       { entity->GetComp<NameComponent>()->GetName() };
    std::set<Transform*> childTransforms   { entity->GetTransform().GetChildren() };
    bool isLeaf                            { childTransforms.empty() };
    
    // Get the selected entity
    ecs::EntityHandle selectedEntity = nullptr;
    selectedEntity = ST<Editor>::Get()->GetSelectedEntity();
    bool isSelected = (entity == selectedEntity);
    // Check if this entity is an ancestor of the selected entity
    // Only do this check if we're not the selected entity itself and there is a selected entity
    bool isAncestorOfSelected = false;
    if (!isSelected && selectedEntity) {
        // Start from the selected entity's parent
        Transform* currentTransform = selectedEntity->GetTransform().GetParent();
        
        // Traverse up the hierarchy
        while (currentTransform && !isAncestorOfSelected) {
            if (currentTransform->GetEntity() == entity) {
                isAncestorOfSelected = true;
            }
            currentTransform = currentTransform->GetParent();
        }
    }

    bool isFilterActive = !textFilter.IsEmpty();
    // Only check for matches if filter is active
    bool matchesSearch = isFilterActive && textFilter.PassFilter(name.c_str());

    // For non-leaf nodes, pre-check if any children match the search
    bool anyChildMatches = false;
    if (isFilterActive && !matchesSearch && !isLeaf) {
        anyChildMatches = DoesEntityOrChildrenMatchSearch(entity, textFilter);
    }
    
    // If filter is active but neither this entity nor any children match, skip rendering
    if (isFilterActive && !matchesSearch && !anyChildMatches) {
        return false;
    }

    // Apply color based on selection and search match
    if (isSelected) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 0.0f, 1.0f, 0.0f, 1.0f });
    } else if (isFilterActive && matchesSearch) {
        // Highlight matching entities with a different color (yellow)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4{ 1.0f, 1.0f, 0.0f, 1.0f });
    }

    // Push ID to avoid ImGui ID conflicts
    ImGui::PushID(id.c_str());
    
    if (isLeaf) // No children
    {
        // We only need to indent leaf nodes
        ImGui::Indent(LEAF_INDENT);

        // Check if this entity is being renamed
        if (isRenamingEntity && entity == entityToRename)
        {
            // Pop colour before renaming widget comes up
            if (isSelected || (isFilterActive && matchesSearch))
            {
                ImGui::PopStyleColor();
            }

            HandleEntityRenaming();
        }
        else // Not renaming, display normally
        {
            // Display
            ImGui::Selectable(name.c_str(), false, ImGuiSelectableFlags_DontClosePopups);

            // Pop colour after displaying
            if (isSelected || (isFilterActive && matchesSearch))
            {
                ImGui::PopStyleColor();
            }
        }

        CheckSelection(entity);
        CheckEntityContextMenu(entity);

        // Unindent after rendering
        ImGui::Unindent(LEAF_INDENT);

        // Check if we're hovering this entity during drag-and-drop
        HandleDragAndDrop(entity, name, targetSceneIndex);
    }
    else // Has children
    {
        // Check if node was expanded before
        bool treeOpen = expandedNodes.contains(entity);
        
        // Force expansion if any child matches the search and filter is active
        if (isFilterActive && anyChildMatches) {
            treeOpen = true;
        }
        
        // Force expansion if this entity is an ancestor of the selected entity
        // This ensures we can see the selected entity by expanding its parents
        if (isAncestorOfSelected) {
            treeOpen = true;
        }
        
        if (isRenamingEntity && entity == entityToRename)
        {
            // Pop colour before renaming widget comes up
            if (isSelected || matchesSearch)
            {
                ImGui::PopStyleColor();
            }
            HandleEntityRenaming();

            ImGui::PopID();
            return matchesSearch || anyChildMatches;
        }
        // Set next item open state
        ImGui::SetNextItemOpen(treeOpen);

        // Create dropdown
        treeOpen = ImGui::TreeNodeEx(id.c_str(), ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth, "%s", name.c_str());

        // Pop colour after displaying
        if (isSelected || matchesSearch)
        {
            ImGui::PopStyleColor();
        }

        // Check if we're hovering this entity during drag-and-drop
        HandleDragAndDrop(entity, name, targetSceneIndex);

        CheckSelection(entity);
        CheckEntityContextMenu(entity);

        // If the tree is open
        if (treeOpen)
        {
            // Add to expanded nodes
            expandedNodes.insert(entity);

            // Iterate children
            for (auto cit = childTransforms.begin(); cit != childTransforms.end(); ++cit)
            {
                ecs::EntityHandle child = (*cit)->GetEntity();
                ShowEntity(child, targetSceneIndex, textFilter);
            }

            ImGui::TreePop();
        }
        else // Tree not open
        {
            expandedNodes.erase(entity);
        }
    }

    // Pop ID after each entity
    ImGui::PopID();
    
    // Return true if this entity or any of its children match the search filter
    // Also return true if this entity is an ancestor of the selected entity to 
    // ensure the ancestor chain is properly rendered
    return matchesSearch || anyChildMatches || isAncestorOfSelected;
}
void Hierarchy::HandleSceneDropTarget(int targetSceneIndex)
{
    // Check if we're in a drag-and-drop operation
    if (ImGui::IsDragDropActive())
    {
        // Get the last child position to draw a drop indicator
        ImVec2 startPos = ImGui::GetItemRectMin();
        ImVec2 endPos = ImGui::GetItemRectMax();

        // Draw a highlight around the drop area when dragging
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRect(
            ImVec2(startPos.x - 2, startPos.y - 2),
            ImVec2(endPos.x + 2, endPos.y + 2),
            IM_COL32(0, 200, 255, 100),
            4.0f,
            0,
            2.0f
        );

        // Add a dashed line to indicate drop area
        const float dashLength = 8.0f;
        float startX = startPos.x + 4;
        float endX = endPos.x - 4;
        float y = startPos.y + 10; // Adjust position based on spacing

        for (float x = startX; x < endX; x += dashLength * 2)
        {
            float lineEnd = x + dashLength;
            if (lineEnd > endX) lineEnd = endX;

            drawList->AddLine(
                ImVec2(x, y),
                ImVec2(lineEnd, y),
                IM_COL32(0, 180, 255, 150),
                2.0f
            );
        }
        
        // Always add text to indicate this is a scene drop target
        // This is important as we're ensuring minimum height for this text
        float availWidth = endPos.x - startPos.x;
        std::string dropText = "Drop here to move to this scene";
        ImVec2 textSize = ImGui::CalcTextSize(dropText.c_str());
        
        // Center the text horizontally and vertically
        float textX = startPos.x + (availWidth - textSize.x) * 0.5f;
        float textY = startPos.y + ((endPos.y - startPos.y) - textSize.y) * 0.5f;
        
        // Ensure text is visible with a slight background for better readability
        drawList->AddRectFilled(
            ImVec2(textX - 5, textY - 3),
            ImVec2(textX + textSize.x + 5, textY + textSize.y + 3),
            IM_COL32(40, 40, 40, 180),
            3.0f
        );
        
        // Draw the text
        drawList->AddText(
            ImVec2(textX, textY),
            IM_COL32(180, 220, 255, 255),
            dropText.c_str()
        );
    }

    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY");
        if (payload != nullptr && payload->DataSize == sizeof(ecs::EntityHandle))
        {
            ecs::EntityHandle draggedEntity = *(ecs::EntityHandle*)payload->Data;
            
            // Get current scene of the entity
            int currentSceneIndex = draggedEntity->GetComp<SceneIndexComponent>()->GetSceneIndex();
            
            // Only perform the move if the scenes are different
            if (currentSceneIndex != targetSceneIndex) {
                // Quick visual feedback animation
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                ImVec2 startPos = ImGui::GetItemRectMin();
                ImVec2 endPos = ImGui::GetItemRectMax();
                drawList->AddRectFilled(
                    startPos,
                    endPos,
                    IM_COL32(0, 255, 100, 80),
                    4.0f
                );

                // Keep the current scene expanded
                expandedSceneIndices.insert(targetSceneIndex);
                
                // Unparent the entity and update its scene
                draggedEntity->GetTransform().SetParent(nullptr);
                SwitchEntityScene(draggedEntity, targetSceneIndex);

                // Update editor's selected entity
                ST<Editor>::Get()->SetSelectedEntity(draggedEntity);
            }
        }
        ImGui::EndDragDropTarget();
    }
}
//
void Hierarchy::HandleDragAndDrop(ecs::EntityHandle entity, std::string const& name, int targetSceneIndex)
{
    // Drag-and-drop source - make this entity draggable
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        // Set payload data
        ImGui::SetDragDropPayload("ENTITY", &entity, sizeof(entity));

        // Display drag feedback with improved visuals
        ImGui::BeginGroup();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        ImGui::Text(ICON_FA_OBJECT_GROUP " %s", name.c_str());
        ImGui::PopStyleColor();

        // Add a tiny preview of entity properties
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::TextUnformatted("Dragging entity...");
        ImGui::PopStyleColor();
        ImGui::EndGroup();

        ImGui::EndDragDropSource();
    }

    // Drag-and-drop target - handle other entities being dropped on this entity
    if (ImGui::BeginDragDropTarget())
    {
        // Get mouse position relative to item for hover feedback
        ImVec2 size = ImGui::GetItemRectSize();
        ImVec2 pos = ImGui::GetItemRectMin();
        ImVec2 mousePos = ImGui::GetMousePos();

        // Draw hover indicator lines based on where the mouse is
        ImDrawList* drawlist = ImGui::GetWindowDrawList();
        int dropMode = 0; // 0 = parent, 1 = above, -1 = below

        if (mousePos.y < pos.y + size.y * 0.2f) // Above entity
        {
            // Draw line with glow effect
            drawlist->AddLine(
                ImVec2(pos.x, pos.y),
                ImVec2(pos.x + size.x, pos.y),
                IM_COL32(100, 180, 255, 200),
                3.0f
            );

            // Add a small triangle indicator
            drawlist->AddTriangleFilled(
                ImVec2(pos.x + 5, pos.y - 3),
                ImVec2(pos.x + 15, pos.y - 3),
                ImVec2(pos.x + 10, pos.y),
                IM_COL32(100, 180, 255, 200)
            );

            dropMode = 1;
        }
        else if (mousePos.y > pos.y + size.y * 0.8f) // Below entity
        {
            // Draw line with glow effect
            drawlist->AddLine(
                ImVec2(pos.x, pos.y + size.y),
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(100, 180, 255, 200),
                3.0f
            );

            // Add a small triangle indicator
            drawlist->AddTriangleFilled(
                ImVec2(pos.x + 5, pos.y + size.y + 3),
                ImVec2(pos.x + 15, pos.y + size.y + 3),
                ImVec2(pos.x + 10, pos.y + size.y),
                IM_COL32(100, 180, 255, 200)
            );

            dropMode = -1;
        }
        else // Middle of entity - parent to this entity
        {
            // Draw a animated pulsing rectangle (approximate animation with transparency)
            float alpha = 0.6f + 0.2f * sinf(static_cast<float>(ImGui::GetTime()) * 8.0f);

            // Outer glow
            drawlist->AddRect(
                ImVec2(pos.x - 2, pos.y - 2),
                ImVec2(pos.x + size.x + 2, pos.y + size.y + 2),
                IM_COL32(100, 255, 100, 100.0f * alpha),
                4.0f,
                0,
                2.0f
            );

            // Inner highlight
            drawlist->AddRectFilled(
                ImVec2(pos.x, pos.y),
                ImVec2(pos.x + size.x, pos.y + size.y),
                IM_COL32(40, 180, 40, 60 * alpha),
                3.0f
            );

            // Show "Parent" text hint
            ImVec2 textSize = ImGui::CalcTextSize("Parent");
            drawlist->AddText(
                ImVec2(pos.x + size.x - textSize.x - 5, pos.y + 2),
                IM_COL32(220, 220, 220, 180),
                "Parent"
            );

            dropMode = 0;
        }

        // Check for payload acceptance
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("ENTITY");
        if (payload != nullptr && payload->DataSize == sizeof(ecs::EntityHandle))
        {
            ecs::EntityHandle draggedEntity = *(ecs::EntityHandle*)payload->Data;

            // Prevent circular parenting
            if (draggedEntity != entity && !IsEntityAncestorOf(draggedEntity, entity))
            {
                // Handle different drop modes
                switch (dropMode)
                {
                case 0: // Parent to this entity
                    // Change scene if needed
                    SwitchEntityScene(draggedEntity, targetSceneIndex);

                    // Set as child of this entity
                    draggedEntity->GetTransform().SetParent(entity->GetTransform());

                    // Expand this node
                    expandedNodes.insert(entity);
                    break;

                case 1: // Above - same parent, different order
                case -1: // Below - same parent, different order
                    // Get the parent of the target entity
                    Transform * parentTransform = entity->GetTransform().GetParent();

                    // Change scene if needed
                    SwitchEntityScene(draggedEntity, targetSceneIndex);

                    // Set the same parent
                    draggedEntity->GetTransform().SetParent(parentTransform);

                    // Note: Reordering within the parent's children set is not implemented
                    // as std::set doesn't support explicit ordering
                    break;
                }

                // Update selected entity
                ST<Editor>::Get()->SetSelectedEntity(draggedEntity);

                // Provide visual feedback on successful drop with a quick flash effect
                drawlist->AddRectFilled(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(100, 255, 100, 120),
                    3.0f
                );
            }
            else
            {
                // Visual feedback for invalid operation
                drawlist->AddRectFilled(
                    pos,
                    ImVec2(pos.x + size.x, pos.y + size.y),
                    IM_COL32(255, 100, 100, 120),
                    3.0f
                );

                // Show error message
                ImVec2 textSize = ImGui::CalcTextSize("Cannot create circular reference");
                drawlist->AddText(
                    ImVec2(pos.x + (size.x - textSize.x) * 0.5f, pos.y + (size.y - textSize.y) * 0.5f),
                    IM_COL32(255, 200, 200, 255),
                    "Cannot create circular reference"
                );
            }
        }

        ImGui::EndDragDropTarget();
    }
}


bool Hierarchy::IsEntityAncestorOf(ecs::EntityHandle potentialAncestor, ecs::EntityHandle entity)
{
    // Get the transform of the entity
    Transform* currentTransform = entity->GetTransform().GetParent();

    // Traverse up the hierarchy
    while (currentTransform != nullptr)
    {
        // Check if this transform belongs to the potential ancestor
        if (currentTransform->GetEntity() == potentialAncestor)
        {
            return true;
        }

        // Move up to the parent
        currentTransform = currentTransform->GetParent();
    }

    // If we reach here, potentialAncestor is not an ancestor of entity
    return false;
}

// Updates which scene an entity is under
void Hierarchy::SwitchEntityScene(ecs::EntityHandle entity, int sceneIndex)
{
	// All entities should have a scene index component
	if(entity->GetComp<SceneIndexComponent>()->GetSceneIndex() != sceneIndex)
		ST<SceneManager>::Get()->SetEntitySceneIndex(entity, sceneIndex);
}

void Hierarchy::HandleEntityRenaming()
{
	// Display input field
	if(ImGui::InputText("##Rename", buffer.data(), buffer.size(),
											ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
	{
		buffer.resize(strlen(buffer.c_str()));
		entityToRename->GetComp<NameComponent>()->SetName(std::string{ buffer });
		isRenamingEntity = false;
		entityToRename = nullptr;
	}

	// Cancel if user clicks outside or presses escape
	if(ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered() ||
		 ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		isRenamingEntity = false;
		entityToRename = nullptr;
	}
}

void Hierarchy::HandleSceneRenaming()
{
	// Display input field
	if(ImGui::InputText("##Rename", buffer.data(), buffer.capacity(),
											ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll))
	{
		buffer.resize(strlen(buffer.data()));
		if(!buffer.empty())
			ST<SceneManager>::Get()->GetSceneAtIndex(sceneIndexToRename)->SetName(buffer);
		sceneIndexToRename = -1;
		isRenamingScene = false;
	}

	// Cancel if user clicks outside or presses escape
	if(ImGui::IsMouseClicked(0) && !ImGui::IsItemHovered() ||
		 ImGui::IsKeyPressed(ImGuiKey_Escape))
	{
		sceneIndexToRename = -1;
		isRenamingScene = false;
	}
}

bool Hierarchy::FindElementInChildren(std::set<Transform*> children, Transform* toFind)
{
	// Check if current set contains the element
	if(children.find(toFind) != children.end())
	{
		return true;
	}
	// Check recursively
	for(auto cit = children.begin(); cit != children.end(); ++cit)
	{
		if(FindElementInChildren((*cit)->GetChildren(), toFind))
		{
			return true;
		}
	}
	return false;
}

void Hierarchy::CheckSelection(ecs::EntityHandle entity)
{
	if(ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		// Set selected entity on single click
#ifdef IMGUI_ENABLED
		ST<Editor>::Get()->SetSelectedEntity(entity);
#endif

    // Trigger message on double click
		if(const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now(); lastClickedEntity == entity && std::chrono::duration<float>(now - lastClickTime).count() <= DBL_CLICK_THRESHOLD)
		{
			Messaging::BroadcastAll("OnEntitySelected", entity);
		}
		else
		{
			lastClickTime = now;
			lastClickedEntity = entity;
		}
	}
}

void Hierarchy::CheckBackgroundWindowContextMenu()
{
	if(ImGui::BeginPopupContextWindow("SceneBackgroundContextMenu"))
	{
		if(ImGui::MenuItem("Create Scene"))
			ST<SceneManager>::Get()->CreateEmptyScene("New Scene");


		if(ImGui::MenuItem("Create Entity"))
			ST<Editor>::Get()->CreateEntityAndSelect();

		ImGui::EndPopup();
	}
}

// Returns true if a scene was unloaded. False otherwise.
bool Hierarchy::CheckSceneContextMenu(int sceneIndex)
{
	ImGui::PushID(("scene" + std::to_string(sceneIndex)).c_str());

	bool wasSceneRemoved{ false };
	if(ImGui::BeginPopupContextItem("SceneContextMenu"))
	{
		if(ImGui::MenuItem("Set Active Scene"))
		{
			ST<SceneManager>::Get()->SetActiveScene(sceneIndex);
		}

		if(ImGui::MenuItem("Rename"))
		{
			sceneIndexToRename = sceneIndex;
			isRenamingScene = true;
			buffer = ST<SceneManager>::Get()->GetSceneAtIndex(sceneIndex)->GetName();
			buffer.resize(32); // Max length of scene name
		}

		if(ImGui::MenuItem("Unload Scene", nullptr, false, ST<SceneManager>::Get()->CheckCanUnloadScene(sceneIndex)))
		{
			ST<SceneManager>::Get()->UnloadScene(sceneIndex);
			wasSceneRemoved = true;
		}

		ImGui::EndPopup();
	}

	ImGui::PopID();
	return wasSceneRemoved;
}

void Hierarchy::CheckEntityContextMenu(ecs::EntityHandle entity)
{
	if(ImGui::BeginPopupContextItem("EntityContextMenu"))
	{
		ST<Editor>::Get()->SetSelectedEntity(entity);

		if(ImGui::MenuItem("Rename"))
		{
			entityToRename = entity;
			isRenamingEntity = true;
			buffer = entity->GetComp<NameComponent>()->GetName();
			buffer.resize(BUFFER_SIZE);
		}

		if(ImGui::MenuItem("Delete Entity"))
			ST<Editor>::Get()->DeleteSelectedEntity();

		ImGui::EndPopup();
	}
}

// Helper function to check if an entity or any of its children match the search filter
bool Hierarchy::DoesEntityOrChildrenMatchSearch(ecs::EntityHandle entity, const gui::TextBoxWithFilter& textFilter)
{
	// Get the entity name
	std::string name = entity->GetComp<NameComponent>()->GetName();

	// Check if this entity matches using the filter
	if(textFilter.PassFilter(name.c_str())) {
		return true;
	}

	// Check all children recursively
	std::set<Transform*> childTransforms = entity->GetTransform().GetChildren();
	for(auto cit = childTransforms.begin(); cit != childTransforms.end(); ++cit) {
		ecs::EntityHandle child = (*cit)->GetEntity();
		if(DoesEntityOrChildrenMatchSearch(child, textFilter)) {
			return true;
		}
	}

	return false;
}
#endif