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


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#ifdef IMGUI_ENABLED
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
#include "../MagicEngine-core/Components/NameComponent.h"

namespace editor {

    void Gizmo::Draw(ecs::EntityHandle selectedEntity)
    {
        // Only enable drawing of the gizmos if there is a selected entity
        ImGuizmo::Enable(selectedEntity);

        if (!selectedEntity)
            return;

        Transform& transform{ selectedEntity->GetTransform() };

        //MUST PUT to allow gizmo to overlap the Image
        ImGui::SetItemAllowOverlap();

        //Compute the rect of the just-drawn Image 
        ImVec2 imgMin = ImGui::GetItemRectMin();
        ImVec2 imgSize = ImGui::GetItemRectSize();

        //One BeginFrame per Dear ImGui frame + enable
        ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
        static int s_lastFrame = -1;
        int curFrame = ImGui::GetFrameCount();
        if (s_lastFrame != curFrame) { ImGuizmo::BeginFrame(); s_lastFrame = curFrame; }

        // 4) Draw into the same window’s drawlist
        ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
        ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);

        // 5) View/Proj 
        glm::mat4 V(1.f), P(1.f);
        bool isOrtho = false;
        if (!EditorCam_TryGet(V, P, isOrtho))
            return;

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
            std::cout << sel->GetComp<NameComponent>()->GetName() << std::endl;
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
        if (ImGuizmo::IsUsing())
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
    }

}

#endif