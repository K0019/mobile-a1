/******************************************************************************/
/*!
\file   video_decoder.h
\brief  Platform-native video decoder for H.264/HEVC playback.

        Uses Windows Media Foundation on Windows and MediaCodec on Android.
        Zero external dependencies - uses OS-provided APIs.
*/
/******************************************************************************/

#pragma once

#ifdef MAGIC_HAS_VIDEO

#include "video_frame.h"
#include <string>
#include <memory>
#include <vector>

namespace video
{
    /**
     * @brief Decoded audio data from video file.
     */
    struct AudioData
    {
        std::vector<int16_t> samples;  // Interleaved PCM samples
        uint32_t sampleRate = 0;       // e.g., 44100, 48000
        uint32_t channels = 0;         // 1 = mono, 2 = stereo
        double duration = 0.0;         // Duration in seconds
    };

    /**
     * @brief Video stream information.
     */
    struct VideoInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;
        double frameRate = 30.0;
        double duration = 0.0;
        bool hasAudio = false;
        std::string codec;
    };

    /**
     * @brief Decoder state.
     */
    enum class DecoderState : uint8_t
    {
        Closed,
        Ready,
        EndOfStream,
        Error
    };

    /**
     * @brief Platform-native video decoder.
     *
     * Windows: Uses Media Foundation (IMFSourceReader)
     * Android: Uses MediaCodec NDK (AMediaExtractor + AMediaCodec)
     */
    class VideoDecoder
    {
    public:
        VideoDecoder();
        ~VideoDecoder();

        // Non-copyable
        VideoDecoder(const VideoDecoder&) = delete;
        VideoDecoder& operator=(const VideoDecoder&) = delete;

        // Movable
        VideoDecoder(VideoDecoder&& other) noexcept;
        VideoDecoder& operator=(VideoDecoder&& other) noexcept;

        /**
         * @brief Open a video file for decoding.
         * @param path File path (UTF-8 on Windows, standard path on Android)
         * @return true if opened successfully
         */
        bool open(const std::string& path);

        /**
         * @brief Open a video from a VFS virtual path.
         * Handles platform differences internally (filesystem path on
         * Windows, file descriptor from APK assets on Android).
         * @param vfsPath Virtual path (e.g. "videos/opening.mp4")
         * @return true if opened successfully
         */
        bool openFromVFS(const std::string& vfsPath);

#ifdef __ANDROID__
        /**
         * @brief Open a video from a file descriptor (Android APK assets).
         * @param fd File descriptor (from AAsset_openFileDescriptor)
         * @param offset Start offset within the fd
         * @param length Length of the asset data
         * @return true if opened successfully
         */
        bool openFd(int fd, off_t offset, off_t length);
#endif

        /**
         * @brief Close the decoder and release resources.
         */
        void close();

        bool isOpen() const { return m_state == DecoderState::Ready; }
        DecoderState getState() const { return m_state; }
        const VideoInfo& getInfo() const { return m_info; }

        /**
         * @brief Decode the next frame.
         * @param outFrame Receives decoded frame data (valid until next call)
         * @return true if frame decoded, false if EOS or error
         */
        bool decodeFrame(VideoFrame& outFrame);

        /**
         * @brief Seek to a timestamp.
         * @param timestamp Time in seconds
         * @return true if seek successful
         */
        bool seek(double timestamp);

        /**
         * @brief Decode all audio from the video file.
         * Call after open(). Decodes entire audio track to PCM.
         * @param outAudio Receives decoded audio data
         * @return true if audio decoded successfully (false if no audio or error)
         */
        bool decodeAllAudio(AudioData& outAudio);

        double getCurrentTime() const { return m_currentTime; }
        const std::string& getLastError() const { return m_lastError; }

    private:
        // Platform-specific implementation (pimpl)
        struct Impl;
        std::unique_ptr<Impl> m_impl;

        VideoInfo m_info;
        DecoderState m_state = DecoderState::Closed;
        double m_currentTime = 0.0;
        std::string m_lastError;

        void setError(const std::string& error);

#ifdef __ANDROID__
        bool setupDecoder();
#endif
    };

} // namespace video

#else // !MAGIC_HAS_VIDEO

// Stub when video support is not available
#include "video_frame.h"
#include <vector>

namespace video
{
    struct AudioData
    {
        std::vector<int16_t> samples;
        uint32_t sampleRate = 0;
        uint32_t channels = 0;
        double duration = 0.0;
    };

    struct VideoInfo
    {
        uint32_t width = 0;
        uint32_t height = 0;
        double frameRate = 30.0;
        double duration = 0.0;
        bool hasAudio = false;
        std::string codec;
    };

    enum class DecoderState : uint8_t
    {
        Closed,
        Ready,
        EndOfStream,
        Error
    };

    class VideoDecoder
    {
    public:
        VideoDecoder() = default;
        bool open(const std::string&) { return false; }
        bool openFromVFS(const std::string&) { return false; }
        void close() {}
        bool isOpen() const { return false; }
        DecoderState getState() const { return DecoderState::Closed; }
        VideoInfo getInfo() const { return {}; }
        bool decodeFrame(VideoFrame&) { return false; }
        bool seek(double) { return false; }
        bool decodeAllAudio(AudioData&) { return false; }
        double getCurrentTime() const { return 0.0; }
        std::string getLastError() const { return "Video support not available"; }
    };
}

#endif // MAGIC_HAS_VIDEO
