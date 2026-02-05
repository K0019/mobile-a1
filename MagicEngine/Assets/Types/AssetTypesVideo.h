/******************************************************************************/
/*!
\file   AssetTypesVideo.h
\brief  Defines the video resource type for the asset system.

        ResourceVideo represents a video asset that can be loaded and played
        using the VideoManager/VideoPlayerComponent systems.
*/
/******************************************************************************/

#pragma once
#include "Assets/AssetBase.h"
#include "video/video_decoder.h"

/**
 * @brief Metadata about a video asset.
 */
struct VideoData
{
    std::string name;               ///< Asset name
    uint32_t width = 0;             ///< Video width in pixels
    uint32_t height = 0;            ///< Video height in pixels
    float duration = 0.0f;          ///< Total duration in seconds
    float frameRate = 30.0f;        ///< Frames per second
    bool hasAudio = false;          ///< Whether video has audio track
    std::string codec;              ///< Codec name (e.g., "h264", "vp9")
};

/**
 * @brief Video resource for the asset system.
 *
 * ResourceVideo stores video metadata and provides access to video playback.
 * The actual video data is loaded and decoded on-demand by VideoManager.
 */
struct ResourceVideo : public ResourceBase
{
    VideoData data;

    /**
     * @brief Check if the video resource has metadata loaded.
     */
    virtual bool IsLoaded() final;

    /**
     * @brief Ensure metadata is loaded, reading from file header if needed.
     * Uses the resource's hash (from ResourceBase) to look up the file path.
     * @return true if metadata is available after this call.
     */
    bool EnsureMetadataLoaded();

    /**
     * @brief Destructor - releases video resources.
     */
    ~ResourceVideo();
};
