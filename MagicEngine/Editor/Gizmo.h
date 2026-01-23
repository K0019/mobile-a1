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

namespace editor {

    /*****************************************************************//*!
    \class Gizmo
    \brief
        Thin wrapper around ImGuizmo that can attach to a
        Transform and draw a manipulator in the viewport’s draw list.
    *//******************************************************************/
    class Gizmo
    {
    public:
        /*****************************************************************//*!
        \brief
             Draw the gizmo to match the scene viewport and allow dragging
        \return
            True if the user is dragging on the gizmos. False otherwise.
        *//******************************************************************/
        bool Draw(ecs::EntityHandle selectedEntity);
    };

}

#endif