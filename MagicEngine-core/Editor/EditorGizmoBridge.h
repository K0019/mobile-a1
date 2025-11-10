/******************************************************************************/
/*!
\file   EditorGizmoBridge.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/10/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
	Declares a tiny header-only bridge for publishing and retrieving the
	editor gizmo operation/mode (used by ImGuizmo)

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#ifdef IMGUI_ENABLED
#include <ImGui/ImguiHeader.h>
#include "ImGuizmo.h"
#endif

/*****************************************************************//*!
\struct EditorGizmoState
\brief
  Stores the current gizmo operation and transform mode for the editor.
*//******************************************************************/
struct EditorGizmoState {
#ifdef IMGUI_ENABLED
	ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE      mode = ImGuizmo::WORLD;
#else
	//set to 0 if imgui disabled
	int op = 0; int mode = 0;
#endif
	bool valid = true;
};

// Header-only global (one definition across TUs thanks to 'inline')
inline EditorGizmoState g_EditorGizmo;

/*****************************************************************//*!
\brief
  Producer API: publish the gizmo operation and mode for this frame.

\param op
  Operation (TRANSLATE/ROTATE/SCALE).
\param mode
  Transform mode (WORLD/LOCAL).
*//******************************************************************/
inline void EditorGizmo_Publish(
#ifdef IMGUI_ENABLED
	ImGuizmo::OPERATION op,
	ImGuizmo::MODE mode
#else
	int op, int mode
#endif
) {
	g_EditorGizmo.op = op;
	g_EditorGizmo.mode = mode;
	g_EditorGizmo.valid = true;
}

#ifdef IMGUI_ENABLED

/*****************************************************************//*!
\brief Consumer API: get current gizmo operation and mode
*//******************************************************************/
inline ImGuizmo::OPERATION EditorGizmo_Op() { return g_EditorGizmo.op; }
inline ImGuizmo::MODE      EditorGizmo_Mode() { return g_EditorGizmo.mode; }
#endif
