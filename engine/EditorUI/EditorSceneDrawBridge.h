#pragma once
#include "imgui.h"

struct EditorSceneDrawArea {
  ImVec2 min{0,0};     // screen-space top-left of the scene image
  ImVec2 size{0,0};    // image size in pixels
  ImDrawList* dl=nullptr;
  bool valid=false;
};

inline EditorSceneDrawArea g_EditorSceneDraw;

inline void EditorScene_PublishDrawArea(ImVec2 min, ImVec2 max, ImDrawList* dl) {
  g_EditorSceneDraw.min = min;
  g_EditorSceneDraw.size = ImVec2(max.x - min.x, max.y - min.y);
  g_EditorSceneDraw.dl = dl;
  g_EditorSceneDraw.valid = (dl != nullptr && g_EditorSceneDraw.size.x > 0 && g_EditorSceneDraw.size.y > 0);
}

inline bool EditorScene_TryGetDrawArea(ImVec2& min, ImVec2& size, ImDrawList*& dl) {
  if (!g_EditorSceneDraw.valid) return false;
  min = g_EditorSceneDraw.min;
  size = g_EditorSceneDraw.size;
  dl = g_EditorSceneDraw.dl;
  return true;
}
