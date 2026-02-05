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


All content � 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "ImGui/ImguiHeader.h"

class RectTransformComponent;

namespace editor {

    /*****************************************************************//*!
    \class Gizmo
    \brief
        Thin wrapper around ImGuizmo that can attach to a
        Transform and draw a manipulator in the viewport�s draw list.
        For UI entities (with RectTransformComponent), draws 2D gizmo instead.
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

    private:
        /*****************************************************************//*!
        \brief
             Draw 2D gizmo for UI entities with RectTransformComponent
        \param selectedEntity
             The entity to draw the 2D gizmo for
        \param rectTransform
             The RectTransformComponent to manipulate
        \param imgMin
             Top-left corner of the viewport image in screen coordinates
        \param imgSize
             Size of the viewport image in screen coordinates
        \return
            True if the user is dragging. False otherwise.
        *//******************************************************************/
        bool Draw2DGizmo(ecs::EntityHandle selectedEntity, RectTransformComponent* rectTransform,
                        ImVec2 imgMin, ImVec2 imgSize);

        // 2D gizmo interaction state
        enum class DragMode { None, Move, ScaleTopLeft, ScaleTopRight, ScaleBottomLeft, ScaleBottomRight };
        DragMode dragMode_ = DragMode::None;
        ImVec2 dragStartMouse_;
        ImVec2 dragStartPos_;
        ImVec2 dragStartScale_;
    };

}