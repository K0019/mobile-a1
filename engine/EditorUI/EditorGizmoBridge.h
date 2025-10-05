#pragma once
#ifdef IMGUI_ENABLED
#include "ImGuizmo.h"
#endif

// Shared editor gizmo state
struct EditorGizmoState {
#ifdef IMGUI_ENABLED
	ImGuizmo::OPERATION op = ImGuizmo::TRANSLATE;
	ImGuizmo::MODE      mode = ImGuizmo::WORLD;
#else
	int op = 0; int mode = 0;
#endif
	bool valid = true;
};

// Header-only global (one definition across TUs thanks to 'inline')
inline EditorGizmoState g_EditorGizmo;

// Helper setters/getters (optional)
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
inline ImGuizmo::OPERATION EditorGizmo_Op() { return g_EditorGizmo.op; }
inline ImGuizmo::MODE      EditorGizmo_Mode() { return g_EditorGizmo.mode; }
#endif
