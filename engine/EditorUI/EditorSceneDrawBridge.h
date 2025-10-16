
/******************************************************************************/
/*!
\file   EditorSceneDrawBridge.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/10/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
	Declares lightweight helpers to publish and retrieve the screen-space
	  draw area (position, size, and draw list) of the Scene viewport

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "imgui.h"

/*****************************************************************//*!
\struct EditorSceneDrawArea
\brief
  The scene image’s screen space rectangle and draw list

*//******************************************************************/
struct EditorSceneDrawArea {
  ImVec2 min{0,0};     // screen-space top-left of the scene image
  ImVec2 size{0,0};    // image size in pixels
  ImDrawList* dl=nullptr;
  bool valid=false;		//!< True if the area was published this frame
};

//Module-scope storage for the current frame’s scene draw area
inline EditorSceneDrawArea g_EditorSceneDraw;

/*****************************************************************//*!

\brief
  Publish the scene image rectangle and draw list for the current ImGui frame.
\param min
  Screen space top left, same as ImGui::GetCursorScreenPos()
\param max
  Screen space bottom right
\param dl
  The ImDrawList of the window that rendered the scene image

*//******************************************************************/

inline void EditorScene_PublishDrawArea(ImVec2 min, ImVec2 max, ImDrawList* dl) {
  g_EditorSceneDraw.min = min;
  g_EditorSceneDraw.size = ImVec2(max.x - min.x, max.y - min.y);
  g_EditorSceneDraw.dl = dl;
  g_EditorSceneDraw.valid = (dl != nullptr && g_EditorSceneDraw.size.x > 0 && g_EditorSceneDraw.size.y > 0);
}

/*****************************************************************//*!

\brief
  Retrieve the previously published scene draw area
\param min
  Screen space top left, same as ImGui::GetCursorScreenPos()
\param max
  Screen space bottom right
\param dl
  The ImDrawList of the window that rendered the scene image

*//******************************************************************/
inline bool EditorScene_TryGetDrawArea(ImVec2& min, ImVec2& size, ImDrawList*& dl) {
  if (!g_EditorSceneDraw.valid) return false;
  min = g_EditorSceneDraw.min;
  size = g_EditorSceneDraw.size;
  dl = g_EditorSceneDraw.dl;
  return true;
}
