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
#include "Gizmo.h"
#include "CustomViewport.h"
#include "imgui_internal.h"
#include "EditorHistory.h"


Gizmo::Gizmo() = default;

Gizmo::~Gizmo() = default;


void Gizmo::attach(Transform& transform)
{
    // If we’re already attached to the same transform, nothing to do.
    if (m_attachedTransform == &transform) {
        return;
    }

    m_attachedTransform = &transform;

}

void Gizmo::detach()
{
    if (!m_attachedTransform)
        return;
    m_attachedTransform = nullptr;
}

void Gizmo::draw([[maybe_unused]] ImDrawList* /*viewport*/) 
{   //skip if no entity selected
    if (!m_attachedTransform) {
        return;
    }

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
    ImGuizmo::Enable(true);

    // 4) Draw into the same window’s drawlist
    ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
    ImGuizmo::SetRect(imgMin.x, imgMin.y, imgSize.x, imgSize.y);

    // 5) View/Proj 
    glm::mat4 V(1.f), P(1.f);
    bool isOrtho = false;
    if (EditorCam_TryGet(V, P, isOrtho))
    {
        ImGuizmo::SetOrthographic(isOrtho);

        float view[16], proj[16];
        std::memcpy(view, glm::value_ptr(V), sizeof(view));
        std::memcpy(proj, glm::value_ptr(P), sizeof(proj));

        // 6) Object/world matrix of selected entity 
        glm::mat4 M(1.f);
        if (auto sel = ST<Inspector>::Get()->GetSelectedEntity())
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
            { snapPtr = snapT; }
            else if (op == ImGuizmo::ROTATE) 
            { snapPtr = &snapDeg; }
            else if (op == ImGuizmo::SCALE) 
            { snapPtr = snapS; }
        }

        // 8) Draw only (no write-back in this pass)
        ImGuizmo::Manipulate(view, proj, op, md, model, nullptr, snapPtr);

        // === WRITE-BACK to Transform when dragging ===
        if (ImGuizmo::IsUsing())
        {
            // Decompose the (possibly modified) model matrix
            float t[3], r[3], s[3];
            ImGuizmo::DecomposeMatrixToComponents(model, t, r, s);

            if (auto sel = ST<Inspector>::Get()->GetSelectedEntity())
            {
                Transform& tr = sel->GetTransform();
                const Transform* parent = tr.GetParent();

                if (md == ImGuizmo::WORLD || !parent)
                {
                    // No parent or WORLD mode -> set world-space directly
                    if (op == ImGuizmo::TRANSLATE) tr.SetWorldPosition({ t[0], t[1], t[2] });
                    else if (op == ImGuizmo::ROTATE) tr.SetWorldRotation({ r[0], r[1], r[2] });  // degrees
                    else if (op == ImGuizmo::SCALE)  tr.SetWorldScale({ s[0], s[1], s[2] });
                }
                else
                {
                    // LOCAL mode with a parent -> convert world -> local first
                    glm::mat4 parentW(1.f);
                    parent->SetMat4ToWorld(&parentW);

                    glm::mat4 worldM = glm::make_mat4(model);
                    glm::mat4 localM = glm::inverse(parentW) * worldM;

                    ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localM), t, r, s);

                    if (op == ImGuizmo::TRANSLATE) tr.SetLocalPosition({ t[0], t[1], t[2] });
                    else if (op == ImGuizmo::ROTATE) tr.SetLocalRotation({ r[0], r[1], r[2] });  // degrees
                    else if (op == ImGuizmo::SCALE)  tr.SetLocalScale({ s[0], s[1], s[2] });
                }
            }
        }

        if (ImGuizmo::IsOver())
            ImGui::SetTooltip("Gizmo: hover");
    }

}

#endif