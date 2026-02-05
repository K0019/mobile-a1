/******************************************************************************/
/*!
\file   video_frame.h
\brief  Defines the VideoFrame struct for decoded video data transfer.

        VideoFrame represents a single decoded video frame ready for GPU upload.
        Supports YUV420P, NV12, and RGBA pixel formats commonly output by
        hardware and software decoders.
*/
/******************************************************************************/

#pragma once
#include <cstdint>

namespace video
{
    /**
     * @brief Pixel format of the decoded frame.
     */
    enum class PixelFormat : uint8_t
    {
        YUV420P,    ///< Planar YUV 4:2:0 (3 separate planes: Y, U, V)
        NV12,       ///< Semi-planar YUV 4:2:0 (2 planes: Y, interleaved UV)
        RGBA,       ///< Pre-converted RGBA (fallback, larger memory footprint)
        Unknown
    };

    /**
     * @brief A decoded video frame ready for rendering.
     *
     * Contains raw pixel data pointers and metadata. The data is owned by the
     * decoder and is only valid until the next decodeFrame() call or decoder
     * destruction.
     */
    struct VideoFrame
    {
        PixelFormat format = PixelFormat::Unknown;

        uint32_t width = 0;         ///< Frame width in pixels
        uint32_t height = 0;        ///< Frame height in pixels

        double pts = 0.0;           ///< Presentation timestamp in seconds
        double duration = 0.0;      ///< Frame duration in seconds

        // Plane data pointers (format-dependent)
        // YUV420P: planes[0]=Y, planes[1]=U, planes[2]=V
        // NV12:    planes[0]=Y, planes[1]=UV interleaved
        // RGBA:    planes[0]=RGBA packed
        const uint8_t* planes[3] = { nullptr, nullptr, nullptr };

        // Stride (bytes per row) for each plane
        uint32_t linesize[3] = { 0, 0, 0 };

        /**
         * @brief Check if this frame contains valid data.
         */
        bool isValid() const
        {
            return width > 0 && height > 0 &&
                   planes[0] != nullptr &&
                   format != PixelFormat::Unknown;
        }

        /**
         * @brief Get the number of planes for the current format.
         */
        uint32_t planeCount() const
        {
            switch (format)
            {
                case PixelFormat::YUV420P: return 3;
                case PixelFormat::NV12:    return 2;
                case PixelFormat::RGBA:    return 1;
                default:                   return 0;
            }
        }

        /**
         * @brief Calculate total frame data size in bytes.
         */
        size_t totalSize() const
        {
            size_t total = 0;
            switch (format)
            {
                case PixelFormat::YUV420P:
                    // Y plane: width * height
                    // U plane: (width/2) * (height/2)
                    // V plane: (width/2) * (height/2)
                    total = static_cast<size_t>(width) * height;
                    total += (static_cast<size_t>(width) / 2) * (height / 2) * 2;
                    break;
                case PixelFormat::NV12:
                    // Y plane: width * height
                    // UV plane: width * (height/2)
                    total = static_cast<size_t>(width) * height;
                    total += static_cast<size_t>(width) * (height / 2);
                    break;
                case PixelFormat::RGBA:
                    total = static_cast<size_t>(width) * height * 4;
                    break;
                default:
                    break;
            }
            return total;
        }
    };

} // namespace video
