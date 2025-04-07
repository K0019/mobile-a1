/******************************************************************************/
/*!
\file   Performance.h
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

#pragma once
#include <psapi.h>
#include "GUICollection.h"
#include "Console.h"

class PerformanceProfiler : public gui::Window {
    public:
    // Enable singleton without exposing constructor/destructor
    friend class ST<PerformanceProfiler>;

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<float>;

    struct SystemProfileDuration
    {
        //! The time that this system took in the last active frame.
        Duration time;
        //! Whether this time is from an old frame.
        bool old;

        /**
         * \brief Constructor.
         */
        SystemProfileDuration();
    };

    /**
     * \brief Starts a new frame for performance profiling.
     */
    void StartFrame();

    /**
     * \brief Starts profiling a specific section of code.
     * \param name The name of the profile section.
     */
    void StartProfile(const std::string& name);

    /**
     * \brief Ends profiling of a specific section of code.
     * \param name The name of the profile section.
     */
    void EndProfile(const std::string& name);

    /**
     * \brief Ends the current frame for performance profiling.
     */
    void EndFrame();

    /**
     * \brief Sleep until next frame.
     * \parram targetFPS
     */
    void WaitTillNextFrame(float targetFPS);

    /**
     * \brief Gets the total frame time in seconds.
     * \return The total frame time in seconds.
     */
    float GetTotalFrameTime() const;

    /**
     * \brief Gets the instantaneous FPS calculated based on the previous frame.
     * \return The FPS.
     */
    float GetFPS() const;

    /**
     * \brief Gets the delta time between frames in seconds.
     * \return The delta time between frames in seconds.
     */
    float GetDeltaTime() const;

    /*****************************************************************//*!
    \brief
        Gets whether this window is open.
    \return
        True if this window is open. False otherwise.
    *//******************************************************************/
    bool GetIsOpen() const;

    /*****************************************************************//*!
    \brief
        Sets whether this window is open.
    \param newIsOpen
        Whether this window is open.
    *//******************************************************************/
    void SetIsOpen(bool newIsOpen);

    /**
     * \brief Draws the performance profiler window.
     */
    void DrawContents() override;

    private:

    PerformanceProfiler();
    PerformanceProfiler(const PerformanceProfiler&) = delete;
    PerformanceProfiler& operator=(const PerformanceProfiler&) = delete;

    struct ProfileData {
        std::string name;
        Duration duration;
    };

    TimePoint startFrameTime;
    TimePoint lastFrameTime;
    Duration totalFrameTime;
    float deltaTime;
    float max_memory;
    std::unordered_map<std::string, TimePoint> startTimes;
    std::unordered_map<std::string, SystemProfileDuration> systemTimes;

    // For framerate graph
    static constexpr size_t MAX_GRAPH_PLOT = 40;
    std::vector<float> fpsGraph;
    std::vector<float> memoryGraph;
    std::vector<float> gpuFrameTimeGraph;

    bool skipFirstFrame = true;

    /**
     * \brief Updates the FPS graph with a new FPS value.
     * \param fps The new FPS value.
     */
    void UpdateFPSGraph(float fps);

    /**
     * \brief Updates the memory graph with a new memory value.
     * \param memory The new memory value.
     */
    void UpdateMemoryGraph(float memory);

    void UpdateGPU(float gpuFrameTime);
};
