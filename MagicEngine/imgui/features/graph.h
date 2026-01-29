#pragma once
#include <deque>
#include <vector>
#include "implot.h"
#include <glm/vec4.hpp>

class LinearGraph
{
public:
  explicit LinearGraph(const char* name, size_t maxGraphPoints = 256) : name_(name), maxPoints_(maxGraphPoints)
  {
  }

  void addPoint(float value)
  {
    graph_.push_back(value);
    if (graph_.size() > maxPoints_) graph_.erase(graph_.begin());
  }

  void renderGraph(uint32_t x, uint32_t y, uint32_t width, uint32_t height,
                   const glm::vec4& color = glm::vec4(1.0)) const
  {
    PROFILER_FUNCTION();
    if (graph_.empty() || width == 0 || height == 0) { return; }
    float minVal = graph_.front();
    float maxVal = graph_.front();
    for (float f : graph_)
    {
      if (f < minVal) minVal = f;
      if (f > maxVal) maxVal = f;
    }
    const float range = maxVal - minVal;
    const bool isConstant = (range == 0.0f); // Check if range is zero
    std::vector<float> dataX_;
    std::vector<float> dataY_;
    size_t numPoints = graph_.size();
    dataX_.reserve(numPoints);
    dataY_.reserve(numPoints);
    for (size_t i = 0; i < numPoints; ++i)
    {
      float f = graph_[i];
      float valY = isConstant ? 0.5f : (f - minVal) / range;
      float valX = static_cast<float>(i) / (maxPoints_ > 1 ? (maxPoints_ - 1) : 1);
      dataX_.push_back(valX);
      dataY_.push_back(valY);
    }
    ImGui::SetNextWindowPos(ImVec2(static_cast<float>(x), static_cast<float>(y)));
    ImGui::SetNextWindowSize(ImVec2(static_cast<float>(width), static_cast<float>(height)));
    ImGui::Begin(name_, nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing
                 | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoBackground | // Keep background transparent
                 ImGuiWindowFlags_NoInputs);
    ImVec2 plotSize = ImGui::GetContentRegionAvail();
    // Ensure plot size isn't negative if window is tiny
    if (plotSize.x <= 0) plotSize.x = 1;
    if (plotSize.y <= 0) plotSize.y = 1;
    if (ImPlot::BeginPlot(name_, plotSize, ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame | ImPlotFlags_NoInputs))
    {
      ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
      ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, 1.0, ImPlotCond_Always);
      ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 1.0, ImPlotCond_Always);
      ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(color.r, color.g, color.b, color.a));
      ImPlot::PushStyleColor(ImPlotCol_PlotBg, ImVec4(0, 0, 0, 0)); // Transparent background
      ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0)); // Remove padding inside plot area
      if (!dataX_.empty())
      {
        ImPlot::PlotLine("##line", dataX_.data(), dataY_.data(), static_cast<int>(numPoints), ImPlotLineFlags_None);
      }
      ImPlot::PopStyleVar();
      ImPlot::PopStyleColor(2);
      ImPlot::EndPlot();
    }
    ImGui::End();
  }

private:
  const char* name_ = nullptr;
  const size_t maxPoints_;
  std::deque<float> graph_;
};
