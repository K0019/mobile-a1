#pragma once
#include <vector>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include "imgui.h"
#include "logging/profiler.h"

class LineCanvas2D
{
public:
  bool isEmpty() const
  {
    return lines_.empty();
  }

  void clear()
  {
    lines_.clear();
  }

  void line(const glm::vec2& p1, const glm::vec2& p2, const glm::vec4& c)
  {
    lines_.push_back({.p1 = p1, .p2 = p2, .color = c});
  }

  void render(const char* nameImGuiWindow)
  {
    PROFILER_FUNCTION();
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::Begin(nameImGuiWindow, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                 ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground |
                 ImGuiWindowFlags_NoInputs);
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    for (const auto& [p1, p2, color] : lines_)
    {
      drawList->AddLine(ImVec2(p1.x, p1.y), ImVec2(p2.x, p2.y), ImColor(color.r, color.g, color.b, color.a));
    }
    ImGui::End();
  }

private:
  struct LineData
  {
    glm::vec2 p1, p2;
    glm::vec4 color;
  };

  std::vector<LineData> lines_;
};
