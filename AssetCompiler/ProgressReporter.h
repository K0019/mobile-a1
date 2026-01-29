/******************************************************************************/
/*!
\file   ProgressReporter.h
\par    Project: Kuro Mahou
\brief  Progress reporting for asset compilation (outputs JSON to stdout).

All content (C) DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include <string>
#include <iostream>

namespace compiler
{

/**
 * @brief Reports compilation progress to stdout as JSON.
 *
 * The editor reads these JSON lines to update a progress bar.
 * Format: {"type":"progress","percent":0.5,"stage":"Compressing","file":"texture.ktx2"}
 */
class ProgressReporter
{
public:
    /**
     * @brief Report progress update.
     * @param percent Progress from 0.0 to 1.0
     * @param stage Current operation description
     * @param file Current file being processed (optional)
     */
    static void ReportProgress(float percent, const std::string& stage, const std::string& file = "")
    {
        // Clamp percent
        if (percent < 0.0f) percent = 0.0f;
        if (percent > 1.0f) percent = 1.0f;

        std::cout << "{\"type\":\"progress\",\"percent\":" << percent
                  << ",\"stage\":\"" << EscapeJson(stage) << "\"";

        if (!file.empty())
        {
            std::cout << ",\"file\":\"" << EscapeJson(file) << "\"";
        }

        std::cout << "}\n";
        std::cout.flush();
    }

    /**
     * @brief Report a message without changing progress.
     * @param message The message to report
     */
    static void ReportMessage(const std::string& message)
    {
        std::cout << "{\"type\":\"message\",\"message\":\"" << EscapeJson(message) << "\"}\n";
        std::cout.flush();
    }

    /**
     * @brief Report completion status.
     * @param success Whether compilation succeeded
     * @param message Optional completion message
     */
    static void ReportComplete(bool success, const std::string& message = "")
    {
        std::cout << "{\"type\":\"complete\",\"success\":" << (success ? "true" : "false");
        if (!message.empty())
        {
            std::cout << ",\"message\":\"" << EscapeJson(message) << "\"";
        }
        std::cout << "}\n";
        std::cout.flush();
    }

private:
    static std::string EscapeJson(const std::string& str)
    {
        std::string result;
        result.reserve(str.size());
        for (char c : str)
        {
            switch (c)
            {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
            }
        }
        return result;
    }
};

/**
 * @brief RAII helper for tracking progress through stages.
 *
 * Usage:
 *   ProgressTracker tracker(3);  // 3 total stages
 *   tracker.BeginStage("Loading model");
 *   // ... do work ...
 *   tracker.BeginStage("Processing textures");
 *   // ... do work ...
 *   tracker.BeginStage("Writing output");
 *   // ... do work ...
 *   tracker.Complete(true);
 */
class ProgressTracker
{
public:
    explicit ProgressTracker(int totalStages)
        : m_totalStages(totalStages)
        , m_currentStage(0)
    {
    }

    void BeginStage(const std::string& stageName, const std::string& file = "")
    {
        m_currentStageName = stageName;
        float progress = static_cast<float>(m_currentStage) / static_cast<float>(m_totalStages);
        ProgressReporter::ReportProgress(progress, stageName, file);
        ++m_currentStage;
    }

    void UpdateStageProgress(float stageProgress, const std::string& file = "")
    {
        // stageProgress is 0-1 within current stage
        float baseProgress = static_cast<float>(m_currentStage - 1) / static_cast<float>(m_totalStages);
        float stageWeight = 1.0f / static_cast<float>(m_totalStages);
        float totalProgress = baseProgress + (stageProgress * stageWeight);
        ProgressReporter::ReportProgress(totalProgress, m_currentStageName, file);
    }

    void Complete(bool success, const std::string& message = "")
    {
        ProgressReporter::ReportProgress(1.0f, success ? "Complete" : "Failed");
        ProgressReporter::ReportComplete(success, message);
    }

private:
    int m_totalStages;
    int m_currentStage;
    std::string m_currentStageName;
};

} // namespace compiler
