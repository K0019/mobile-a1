/******************************************************************************/
/*!
\file   Gizmo.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/10/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    Definition of Gizmo class


All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#include <glm/ext.hpp>
#include "Editor/Gizmo.h"
#include "ImGui/ImguiHeader.h"
#include "ImGuizmo.h"
#include "Editor/EditorCameraBridge.h"   // for EditorCam_TryGet(...)
#include "Editor/EditorGizmoBridge.h"

#include "Engine/Input.h"
#include "Editor/EditorHistory.h"
#include "Engine/Events/EventsQueue.h"
#include "Engine/Events/EventsTypeEditor.h"
#include "Components/NameComponent.h"
#include "UI/RectTransform.h"
#include "renderer/render_graph.h"

namespace editor {

    bool Gizmo::Draw(ecs::EntityHandle selectedEntity)
    {
        // Check if gizmos are globally enabled
        if (!EditorGizmo_Enabled())
        {
            ImGuizmo::Enable(false);
            return false;
        }

        // Only enable drawing of the gizmos if there is a selected entity
        ImGuizmo::Enable(selectedEntity);

        if (!selectedEntity)
            return false;

        //MUST PUT to allow gizmo to overlap the Image
        ImGui::SetItemAllowOverlap();

        //Compute the rect of the just-drawn Image
        ImVec2 imgMin = ImGui::GetItemRectMin();
        ImVec2 imgSize = ImGui::GetItemRectSize();

        // Check if entity has RectTransformComponent (UI entity)
        // If so, use 2D gizmo instead of 3D ImGuizmo
        if (RectTransformComponent* rectTransform = selectedEntity->GetComp<RectTransformComponent>())
        {
            ImGuizmo::Enable(false);  // Disable 3D gizmo
            return Draw2DGizmo(selectedEntity, rectTransform, imgMin, imgSize);
        }

        Transform& transform{ selectedEntity->GetTransform() };

        //One BeginFrame per Dear ImGui frame + enable
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
        static int s_lastFrame = -1;
        int curFrame = ImGui::GetFrameCount();
        if (s_lastFrame != curFrame) { ImGuizmo::BeginFrame(); s_lastFrame = curFrame; }

        // 4) Draw into the same window�s drawlist
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);

        // 5) View/Proj
        glm::mat4 V(1.f), P(1.f);
        bool isOrtho = false;
        if (!EditorCam_TryGet(V, P, isOrtho))
            return false;

        ImGuizmo::SetOrthographic(isOrtho);

        float view[16], proj[16];
        std::memcpy(view, glm::value_ptr(V), sizeof(view));
        std::memcpy(proj, glm::value_ptr(P), sizeof(proj));

        // 6) Object/world matrix of selected entity
        glm::mat4 M(1.f);
        if (auto sel = ST<EventsQueue>::Get()->RequestValueFromEventHandlers<ecs::EntityHandle>(Getters::EditorSelectedEntity{}).value_or(nullptr))
        {
            Transform& tr = sel->GetTransform();
            tr.SetMat4ToWorld(&M);
            ImGuizmo::SetID((int)(intptr_t)sel);
        }
        float model[16];
        std::memcpy(model, glm::value_ptr(M), sizeof(model));

        // 7) Operation and mode
        ImGuizmo::OPERATION op = EditorGizmo_Op();
        ImGuizmo::MODE      md = EditorGizmo_Mode(); //curr using default

        //Snapping controls (Ctrl)
        const bool snapOn = ImGui::GetIO().KeyCtrl;
        float snapT[3] = { 0.5f, 0.5f, 0.5f };  // meters/units
        float snapS[3] = { 0.1f, 0.1f, 0.1f };  // scale step
        float snapDeg = 15.0f;                 // degrees
        const float* snapPtr = nullptr;
        if (snapOn) {
            if (op == ImGuizmo::TRANSLATE)
            {
                snapPtr = snapT;
            }
            else if (op == ImGuizmo::ROTATE)
            {
                snapPtr = &snapDeg;
            }
            else if (op == ImGuizmo::SCALE)
            {
                snapPtr = snapS;
            }
        }

        // 8) Draw only (no write-back in this pass)
        ImGuizmo::Manipulate(view, proj, op, md, model, nullptr, snapPtr);

        // === WRITE-BACK to Transform when dragging ===
        bool isUsing{ ImGuizmo::IsUsing() };
        if (isUsing)
        {
            // Decompose the (possibly modified) model matrix
            float t[3], r[3], s[3];
            ImGuizmo::DecomposeMatrixToComponents(model, t, r, s);

            const Transform* parent = transform.GetParent();

            if (md == ImGuizmo::WORLD || !parent)
            {
                // No parent or WORLD mode -> set world-space directly
                if (op == ImGuizmo::TRANSLATE) {
                    ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ selectedEntity, transform.GetLocalPosition() });
                    transform.SetWorldPosition({ t[0], t[1], t[2] });
                }
                else if (op == ImGuizmo::ROTATE) {
                    ST<History>::Get()->IntermediateEvent(HistoryEvent_Rotation{ selectedEntity, transform.GetLocalRotation() });
                    transform.SetWorldRotation({ r[0], r[1], r[2] });  // degrees
                }
                else if (op == ImGuizmo::SCALE) {
                    ST<History>::Get()->IntermediateEvent(HistoryEvent_Scale{ selectedEntity, transform.GetLocalScale() });
                    transform.SetWorldScale({ s[0], s[1], s[2] });
                }
            }
            else
            {
                // LOCAL mode with a parent -> convert world -> local first
                glm::mat4 parentW(1.f);
                parent->SetMat4ToWorld(&parentW);

                glm::mat4 worldM = glm::make_mat4(model);
                glm::mat4 localM = glm::inverse(parentW) * worldM;

                ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localM), t, r, s);

                if (op == ImGuizmo::TRANSLATE) transform.SetLocalPosition({ t[0], t[1], t[2] });
                else if (op == ImGuizmo::ROTATE) transform.SetLocalRotation({ r[0], r[1], r[2] });  // degrees
                else if (op == ImGuizmo::SCALE)  transform.SetLocalScale({ s[0], s[1], s[2] });
            }
        }

        if (ImGuizmo::IsOver())
            ImGui::SetTooltip("Gizmo: hover");

        return isUsing;
    }

    bool Gizmo::Draw2DGizmo(ecs::EntityHandle selectedEntity, RectTransformComponent* rectTransform,
                           ImVec2 imgMin, ImVec2 imgSize)
    {
        // UI coordinate system: height = 1080, width scales with aspect ratio
        const float viewportAspect = imgSize.x / imgSize.y;
        const float uiWidth = RenderResources::GetUIViewportWidth(viewportAspect);
        const float uiHeight = RenderResources::UI_REFERENCE_HEIGHT;

        // Scale factors: convert UI coords to viewport/screen coords
        const float scaleX = imgSize.x / uiWidth;
        const float scaleY = imgSize.y / uiHeight;

        // Get current position and scale from RectTransform
        Vec2 uiPos = rectTransform->GetWorldPosition();
        Vec2 uiScale = rectTransform->GetWorldScale();

        // Default size for UI elements (can be overridden later with component-specific sizes)
        const float defaultWidth = 1.0f;
        const float defaultHeight = 1.0f;
        float rectWidth = defaultWidth * uiScale.x;
        float rectHeight = defaultHeight * uiScale.y;

        // Convert UI position to screen coordinates
        // UI Y=0 is top, screen Y=0 is also top for ImGui
        float screenX = imgMin.x + uiPos.x * scaleX;
        float screenY = imgMin.y + uiPos.y * scaleY;

        // Rectangle bounds (centered on position)
        float halfW = rectWidth * scaleX * 0.5f;
        float halfH = rectHeight * scaleY * 0.5f;

        ImVec2 rectMin(screenX - halfW, screenY - halfH);
        ImVec2 rectMax(screenX + halfW, screenY + halfH);

        // Handle size for corner scale handles
        const float handleRadius = 6.0f;
        const float handleHitRadius = 10.0f;

        // Colors
        const ImU32 outlineColor = IM_COL32(255, 165, 0, 255);           // Orange
        const ImU32 outlineHoverColor = IM_COL32(255, 200, 100, 255);    // Light orange (hover)
        const ImU32 fillHoverColor = IM_COL32(255, 165, 0, 40);          // Semi-transparent orange fill
        const ImU32 fillDragColor = IM_COL32(255, 165, 0, 60);           // Slightly more opaque when dragging
        const ImU32 handleColor = IM_COL32(255, 255, 255, 255);          // White
        const ImU32 handleHoverColor = IM_COL32(255, 200, 100, 255);     // Light orange

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImGuiIO& io = ImGui::GetIO();

        // Mouse position
        ImVec2 mousePos = io.MousePos;

        // Handle positions (corners)
        ImVec2 handles[4] = {
            ImVec2(rectMin.x, rectMin.y),  // Top-left
            ImVec2(rectMax.x, rectMin.y),  // Top-right
            ImVec2(rectMin.x, rectMax.y),  // Bottom-left
            ImVec2(rectMax.x, rectMax.y)   // Bottom-right
        };

        DragMode handleModes[4] = {
            DragMode::ScaleTopLeft,
            DragMode::ScaleTopRight,
            DragMode::ScaleBottomLeft,
            DragMode::ScaleBottomRight
        };

        // Check hover states for visual feedback
        bool hoveringHandle = false;
        int hoveredHandleIndex = -1;
        for (int i = 0; i < 4; ++i)
        {
            float dx = mousePos.x - handles[i].x;
            float dy = mousePos.y - handles[i].y;
            if (dx * dx + dy * dy <= handleHitRadius * handleHitRadius)
            {
                hoveringHandle = true;
                hoveredHandleIndex = i;
                break;
            }
        }

        bool hoveringRect = !hoveringHandle &&
            mousePos.x >= rectMin.x && mousePos.x <= rectMax.x &&
            mousePos.y >= rectMin.y && mousePos.y <= rectMax.y;

        bool isUsing = false;

        // Check for interaction start (no viewport restriction - gizmo can be anywhere)
        if (io.MouseClicked[0] && dragMode_ == DragMode::None)
        {
            // Check corner handles first (scale)
            if (hoveringHandle && hoveredHandleIndex >= 0)
            {
                dragMode_ = handleModes[hoveredHandleIndex];
                dragStartMouse_ = mousePos;
                dragStartPos_ = ImVec2(uiPos.x, uiPos.y);
                dragStartScale_ = ImVec2(uiScale.x, uiScale.y);
                ST<History>::Get()->IntermediateEvent(HistoryEvent_Scale{ selectedEntity, ecs::GetEntityTransform(rectTransform).GetLocalScale() });
            }
            // Check rect interior (move)
            else if (hoveringRect)
            {
                dragMode_ = DragMode::Move;
                dragStartMouse_ = mousePos;
                dragStartPos_ = ImVec2(uiPos.x, uiPos.y);
                dragStartScale_ = ImVec2(uiScale.x, uiScale.y);
                ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ selectedEntity, ecs::GetEntityTransform(rectTransform).GetLocalPosition() });
            }
        }

        // Process drag (continues even when mouse is outside viewport)
        if (dragMode_ != DragMode::None)
        {
            isUsing = true;

            if (io.MouseDown[0])
            {
                float deltaX = (mousePos.x - dragStartMouse_.x) / scaleX;
                float deltaY = (mousePos.y - dragStartMouse_.y) / scaleY;

                if (dragMode_ == DragMode::Move)
                {
                    // Move: update position
                    rectTransform->SetWorldPosition(Vec2{ dragStartPos_.x + deltaX, dragStartPos_.y + deltaY });
                }
                else
                {
                    // Scale: adjust scale based on which corner is being dragged
                    // Calculate scale change relative to center
                    float scaleChangeX = 0.0f;
                    float scaleChangeY = 0.0f;

                    switch (dragMode_)
                    {
                    case DragMode::ScaleTopLeft:
                        scaleChangeX = -deltaX / (defaultWidth * 0.5f);
                        scaleChangeY = -deltaY / (defaultHeight * 0.5f);
                        break;
                    case DragMode::ScaleTopRight:
                        scaleChangeX = deltaX / (defaultWidth * 0.5f);
                        scaleChangeY = -deltaY / (defaultHeight * 0.5f);
                        break;
                    case DragMode::ScaleBottomLeft:
                        scaleChangeX = -deltaX / (defaultWidth * 0.5f);
                        scaleChangeY = deltaY / (defaultHeight * 0.5f);
                        break;
                    case DragMode::ScaleBottomRight:
                        scaleChangeX = deltaX / (defaultWidth * 0.5f);
                        scaleChangeY = deltaY / (defaultHeight * 0.5f);
                        break;
                    default:
                        break;
                    }

                    float newScaleX = std::max(0.1f, dragStartScale_.x + scaleChangeX);
                    float newScaleY = std::max(0.1f, dragStartScale_.y + scaleChangeY);
                    rectTransform->SetWorldScale(Vec2{ newScaleX, newScaleY });
                }
            }
            else
            {
                // Mouse released, end drag
                dragMode_ = DragMode::None;
            }
        }

        // Draw filled rectangle when hovering or dragging (visual feedback)
        if (dragMode_ == DragMode::Move)
        {
            drawList->AddRectFilled(rectMin, rectMax, fillDragColor);
        }
        else if (hoveringRect)
        {
            drawList->AddRectFilled(rectMin, rectMax, fillHoverColor);
        }

        // Draw rectangle outline (brighter when hovering)
        ImU32 currentOutlineColor = (hoveringRect || dragMode_ == DragMode::Move) ? outlineHoverColor : outlineColor;
        drawList->AddRect(rectMin, rectMax, currentOutlineColor, 0.0f, 0, 2.0f);

        // Draw corner handles
        for (int i = 0; i < 4; ++i)
        {
            bool handleHovered = (i == hoveredHandleIndex) ||
                                 (dragMode_ == handleModes[i]);  // Also highlight if currently dragging this handle

            ImU32 color = handleHovered ? handleHoverColor : handleColor;
            drawList->AddCircleFilled(handles[i], handleRadius, color);
            drawList->AddCircle(handles[i], handleRadius, handleHovered ? outlineHoverColor : outlineColor, 0, 2.0f);
        }

        // Draw center crosshair
        drawList->AddLine(ImVec2(screenX - 10, screenY), ImVec2(screenX + 10, screenY), currentOutlineColor, 1.0f);
        drawList->AddLine(ImVec2(screenX, screenY - 10), ImVec2(screenX, screenY + 10), currentOutlineColor, 1.0f);

        return isUsing;
    }

}