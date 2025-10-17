#pragma once
/******************************************************************************/
/*!
\file   Gizmo.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/10/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    Class that handles Gizmo operations for the editor. Implemented with Imguizmo.


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#ifdef IMGUI_ENABLED
#include "ImGui/ImguiHeader.h"
#include "ImGuizmo.h"
#include "Editor/EditorCameraBridge.h"   // for EditorCam_TryGet(...)
#include "Editor/EditorGizmoBridge.h"


/*****************************************************************//*!
\class Gizmo
\brief
    Thin wrapper around ImGuizmo that can attach to a 
    Transform and draw a manipulator in the viewport’s draw list.
*//******************************************************************/
class Gizmo {
public:

/*****************************************************************//*!
\brief
    Constructor and Destructor
*//******************************************************************/
    Gizmo();
    ~Gizmo();

    /*****************************************************************//*!
    \brief
        Attach this gizmo to a Transform so subsequent draw calls can
        be drag and render
    \param transform
        The Transform to attach. Must remain valid while attached.
    *//******************************************************************/

    void Attach(Transform& transform);
    /*****************************************************************//*!
    \brief
         Detach the transform so it stops rendering and dragging
    *//******************************************************************/

    void Detach();
    /*****************************************************************//*!
    \brief
         Draw the gizmo to match the scene viewport and allow dragging
    *//******************************************************************/
    void Draw(ImDrawList* viewport);

private:
    Transform* m_attachedTransform = nullptr;
};
#endif