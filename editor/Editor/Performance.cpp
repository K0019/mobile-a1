/******************************************************************************/
/*!
\file   Performance.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the PerformanceProfiler class.
The PerformanceProfiler class is responsible for measuring and profiling the performance of the application.
It provides functions to start and end frames, start and end profiles, and draw performance graphs.
It also includes private helper functions for GPU profiling and updating the performance graphs.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "Editor/Performance.h"
#ifdef GLFW
#include <GLFW/glfw3.h>
#include <Windows.h>
#include <Psapi.h>
#endif

// Thanks microsoft
#ifdef max
#undef max
#endif
#ifdef DrawText
#undef DrawText
#endif

namespace
{
    size_t GetMemoryUsage()
    {
#ifdef GLFW
        PROCESS_MEMORY_COUNTERS_EX pmc;
        // Get the handle to the current process
        HANDLE process = GetCurrentProcess();

        // Use GetProcessMemoryInfo to fill the pmc structure with memory info
        if(GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            // Return the working set size, which is the amount of memory used by the process (in bytes)
            return pmc.WorkingSetSize;
        }
#endif

        return 0; // Return 0 if memory retrieval failed
    }
}

PerformanceProfiler::SystemProfileDuration::SystemProfileDuration()
    : time{}
    , old{ true }
{
}

PerformanceProfiler::PerformanceProfiler()
    : deltaTime{}
    , totalFrameTime{}
{
}

void PerformanceProfiler::StartFrame()
{
    startFrameTime = Clock::now();
#ifdef GLFW // TODO: Implement for android
    static float prev_time = static_cast<float>(glfwGetTime());
    const float curr_time = static_cast<float>(glfwGetTime());
    deltaTime = curr_time - prev_time;
    prev_time = curr_time;

    for (auto& [_, duration] : systemTimes)
        duration.old = true;
#endif
}

void PerformanceProfiler::StartProfile(const std::string& name)
{
    startTimes[name] = Clock::now();
}

void PerformanceProfiler::EndProfile(const std::string& name)
{
    auto& profileDuration{ systemTimes[name] };
    if (profileDuration.old)
    {
        profileDuration.time = profileDuration.time.zero();
        profileDuration.old = false;
    }

    auto endTime = Clock::now();
    auto& startTime = startTimes[name];
    profileDuration.time += std::chrono::duration_cast<Duration>(endTime - startTime);
}

void PerformanceProfiler::EndFrame()
{
    totalFrameTime = std::chrono::duration_cast<Duration>(Clock::now() - startFrameTime);
}

void PerformanceProfiler::WaitTillNextFrame(float targetFPS)
{
    auto targetFrameTime{ std::chrono::round<std::chrono::microseconds>(Duration{ 1.0f / targetFPS }) };
    targetFrameTime -= std::chrono::microseconds{ 10 }; // Run slightly faster than target framerate.
    auto targetEndTimepoint{ startFrameTime + targetFrameTime };

    auto currTimepoint{ Clock::now() };

    if(currTimepoint >= targetEndTimepoint)
        return;

    // TODO: Figure out how to make thread scheduling more precise than 16ms
    // Resource that may help: https://stackoverflow.com/questions/73963381/high-precision-sleep-for-values-around-5-ms
    // 
    // Sleep thread until it's almost time to start the next frame.
    //auto microSecsToWait{ std::chrono::duration_cast<std::chrono::microseconds>(targetEndTimepoint - currTimepoint) };
    //microSecsToWait -= std::chrono::microseconds{ 10000 }; // Buffer time for scheduling.
    //std::cout << "Microseconds to wait: " << std::right << std::setw(9) << microSecsToWait.count() << std::endl;
    //if (microSecsToWait.count() >= 0)
    //    std::this_thread::sleep_for(microSecsToWait);
    //std::cout << "Current Time:     " << Clock::now().time_since_epoch() << std::endl;

    // Keep checking time until it's time for the next frame.
    do
    {
        currTimepoint = Clock::now();
    } while(currTimepoint < targetEndTimepoint);
}

float PerformanceProfiler::GetTotalFrameTime() const
{
    return totalFrameTime.count();
}

float PerformanceProfiler::GetFPS() const
{
    return 1.0f / std::chrono::duration_cast<Duration>(Clock::now() - startFrameTime).count();
}

float PerformanceProfiler::GetDeltaTime() const
{
    return deltaTime;
}

const PerformanceProfiler::SystemTimesContType& PerformanceProfiler::GetSystemTimes() const
{
    return systemTimes;
}

namespace editor {

    PerformanceWindow::PerformanceWindow()
        : WindowBase{ ICON_FA_GAUGE_HIGH" Performance", gui::Vec2{ 250, 500 } }
        , skipFirstFrame{ true }
    {
        fpsGraph.reserve(MAX_GRAPH_PLOT);
        memoryGraph.reserve(MAX_GRAPH_PLOT);
    }

    void PerformanceWindow::DrawWindow()
    {
    #ifdef IMGUI_ENABLED
        if (skipFirstFrame) {
            skipFirstFrame = false;
            return;
        }

        ImGuiIO& io = ImGui::GetIO();
        float currentFPS = io.Framerate;
        float frameTime = 1000.0f / io.Framerate;
        UpdateFPSGraph(currentFPS);
        size_t memoryUsage = GetMemoryUsage();
        float memoryUsageMB = memoryUsage / (1024.0f * 1024.0f);
        UpdateMemoryGraph(memoryUsageMB);

        const gui::Vec2 graphSize{ 0.0f, 100.0f };
        if (gui::CollapsingHeader fpsHeader{ "FPS / Frame Time", gui::FLAG_TREE_NODE::DEFAULT_OPEN })
        {
            gui::TextFormatted("Current: %.1f FPS (%.3f ms/frame)", currentFPS, frameTime);
            gui::TextUnformatted("FPS History");
            gui::PlotLines("##FPSGraph", fpsGraph, graphSize, 0.0f, 120.0f);

            gui::TextUnformatted("Frame Time Breakdown:");
            gui::Indent indent{};

            const float totalFrameTime{ ST<PerformanceProfiler>::Get()->GetTotalFrameTime() };
            for (const auto& profile : ST<PerformanceProfiler>::Get()->GetSystemTimes())
            {
                float percentage{ (profile.second.time.count() / totalFrameTime) * 100.0f };
                gui::TextFormatted("%s: %.2f%% (%.3f ms)", profile.first.c_str(), percentage, profile.second.time.count() * 1000.0f);
            }
        }

        if (gui::CollapsingHeader memoryHeader{ "Memory Usage", gui::FLAG_TREE_NODE::DEFAULT_OPEN })
        {
            gui::TextFormatted("Current: %.2f MB", memoryUsageMB);
            gui::TextFormatted("Peak: %.2f MB", max_memory);
            gui::TextUnformatted("Memory History");
            gui::PlotLines("##MemoryGraph", memoryGraph, graphSize, 0.0f, 500.0f);
        }

        if (gui::CollapsingHeader gpuFpsHeader{ "GPU Frame Time", gui::FLAG_TREE_NODE::DEFAULT_OPEN })
        {
            //auto frameStats = VulkanManager::Get().VkQueryManager().GetFrameStats();
            //UpdateGPU(frameStats.frameTime);
            if (!gpuFrameTimeGraph.empty())
            {
                /*ImGui::Text("Frame Time: %.3f ms", frameStats.frameTime);
                ImGui::Text("Vertices: %llu", frameStats.vertexCount);
                ImGui::Text("Primitives: %llu", frameStats.primitiveCount);*/
                gui::Separator();

                gui::TextUnformatted("GPU Frame Time History");

                constexpr float maxGPUFrameTime = 33.33f; //30 FPS

                gui::PlotLines("##GPUFrameTimeGraph", gpuFrameTimeGraph, graphSize, 0.0f, maxGPUFrameTime * 1.1f);

                gui::Vec2 p_min = gui::GetPrevItemRectMin();
                gui::Vec2 p_max = gui::GetPrevItemRectMax();
                float h = p_max.y - p_min.y;

                auto addFrameTimeLine = [&](float ms, const char* label, const gui::Vec4& color) {
                    float y = p_max.y - (ms / maxGPUFrameTime) * h;
                    gui::DrawLine(gui::Vec2{ p_min.x, y }, gui::Vec2{ p_max.x, y }, color);
                    gui::DrawText(label, gui::Vec2{ p_max.x + 5.0f, y - 7.0f }, color);
                };

                addFrameTimeLine(16.67f, "60 FPS", gui::Vec4{ 0, 255, 0, 200 });  // Green
                addFrameTimeLine(33.33f, "30 FPS", gui::Vec4{ 255, 255, 0, 200 });  // Yellow
            }
            else
                gui::TextUnformatted("No GPU frame time data available yet.");
        }
    #endif
    }

    void PerformanceWindow::UpdateFPSGraph(float fps)
    {
        fpsGraph.push_back(fps);
        if (fpsGraph.size() > MAX_GRAPH_PLOT)
            fpsGraph.erase(fpsGraph.begin());
    }

    void PerformanceWindow::UpdateMemoryGraph(float memory)
    {
        memoryGraph.push_back(memory);
        max_memory = std::max(memory, max_memory);
        if (memoryGraph.size() > MAX_GRAPH_PLOT)
            memoryGraph.erase(memoryGraph.begin());
    }

    void PerformanceWindow::UpdateGPU(float gpuFrameTime)
    {
        gpuFrameTimeGraph.push_back(gpuFrameTime);
        if (gpuFrameTimeGraph.size() > MAX_GRAPH_PLOT)
            gpuFrameTimeGraph.erase(gpuFrameTimeGraph.begin());
    }

}
