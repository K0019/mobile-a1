/******************************************************************************/
/*!
\file   video_decoder_android.cpp
\brief  Android MediaCodec video decoder implementation.
*/
/******************************************************************************/

#ifdef MAGIC_HAS_VIDEO
#ifdef __ANDROID__

#include "video_decoder.h"
#include "VFS/VFS.h"

#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaFormat.h>

#include <vector>
#include <android/log.h>

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "VideoDecoder", __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, "VideoDecoder", __VA_ARGS__)

namespace video
{
    // Platform-specific implementation
    struct VideoDecoder::Impl
    {
        AMediaExtractor* extractor = nullptr;
        AMediaCodec* codec = nullptr;
        AMediaFormat* format = nullptr;

        std::string filePath;  // Store for audio decoding (filesystem path)
        int assetFd = -1;     // Store for audio decoding (APK fd-based)
        off_t assetOffset = 0;
        off_t assetLength = 0;
        int32_t videoTrackIndex = -1;
        int32_t audioTrackIndex = -1;

        // Current decoded frame buffer (typically NV12 or YUV420P)
        std::vector<uint8_t> frameBuffer;
        uint32_t frameWidth = 0;
        uint32_t frameHeight = 0;
        uint32_t frameStride = 0;
        uint32_t sliceHeight = 0;  // Actual buffer height (may be > frameHeight due to alignment)
        int32_t colorFormat = 0;

        bool inputEOS = false;
        bool outputEOS = false;

        ~Impl()
        {
            release();
        }

        void release()
        {
            if (codec)
            {
                AMediaCodec_stop(codec);
                AMediaCodec_delete(codec);
                codec = nullptr;
            }
            if (format)
            {
                AMediaFormat_delete(format);
                format = nullptr;
            }
            if (extractor)
            {
                AMediaExtractor_delete(extractor);
                extractor = nullptr;
            }
            frameBuffer.clear();
            filePath.clear();
            assetFd = -1;
            assetOffset = 0;
            assetLength = 0;
            videoTrackIndex = -1;
            audioTrackIndex = -1;
            inputEOS = false;
            outputEOS = false;
        }
    };

    VideoDecoder::VideoDecoder()
        : m_impl(std::make_unique<Impl>())
    {
    }

    VideoDecoder::~VideoDecoder()
    {
        close();
    }

    VideoDecoder::VideoDecoder(VideoDecoder&& other) noexcept
        : m_impl(std::move(other.m_impl))
        , m_info(std::move(other.m_info))
        , m_state(other.m_state)
        , m_currentTime(other.m_currentTime)
        , m_lastError(std::move(other.m_lastError))
    {
        other.m_state = DecoderState::Closed;
    }

    VideoDecoder& VideoDecoder::operator=(VideoDecoder&& other) noexcept
    {
        if (this != &other)
        {
            close();
            m_impl = std::move(other.m_impl);
            m_info = std::move(other.m_info);
            m_state = other.m_state;
            m_currentTime = other.m_currentTime;
            m_lastError = std::move(other.m_lastError);
            other.m_state = DecoderState::Closed;
        }
        return *this;
    }

    bool VideoDecoder::openFromVFS(const std::string& vfsPath)
    {
        int fd = -1;
        off_t offset = 0, length = 0;
        if (!VFS::GetFileDescriptor(vfsPath, fd, offset, length))
        {
            setError("Failed to get file descriptor for: " + vfsPath);
            return false;
        }
        return openFd(fd, offset, length);
    }

    bool VideoDecoder::open(const std::string& path)
    {
        close();

        m_impl->filePath = path;

        // Create extractor
        m_impl->extractor = AMediaExtractor_new();
        if (!m_impl->extractor)
        {
            setError("Failed to create media extractor");
            return false;
        }

        // Set data source from filesystem path
        media_status_t status = AMediaExtractor_setDataSource(m_impl->extractor, path.c_str());
        if (status != AMEDIA_OK)
        {
            setError("Failed to set data source: " + path);
            close();
            return false;
        }

        return setupDecoder();
    }

    bool VideoDecoder::openFd(int fd, off_t offset, off_t length)
    {
        close();

        m_impl->assetFd = fd;
        m_impl->assetOffset = offset;
        m_impl->assetLength = length;

        // Create extractor
        m_impl->extractor = AMediaExtractor_new();
        if (!m_impl->extractor)
        {
            setError("Failed to create media extractor");
            return false;
        }

        // Set data source from file descriptor (APK assets)
        media_status_t status = AMediaExtractor_setDataSourceFd(
            m_impl->extractor, fd, offset, length);
        if (status != AMEDIA_OK)
        {
            setError("Failed to set data source from fd");
            close();
            return false;
        }

        return setupDecoder();
    }

    // Shared setup after data source is configured on the extractor
    bool VideoDecoder::setupDecoder()
    {
        // Find video track
        int numTracks = AMediaExtractor_getTrackCount(m_impl->extractor);
        for (int i = 0; i < numTracks; i++)
        {
            AMediaFormat* trackFormat = AMediaExtractor_getTrackFormat(m_impl->extractor, i);
            const char* mime = nullptr;
            AMediaFormat_getString(trackFormat, AMEDIAFORMAT_KEY_MIME, &mime);

            if (mime && strncmp(mime, "video/", 6) == 0)
            {
                m_impl->videoTrackIndex = i;
                m_impl->format = trackFormat;

                // Extract video info
                int32_t width = 0, height = 0;
                AMediaFormat_getInt32(trackFormat, AMEDIAFORMAT_KEY_WIDTH, &width);
                AMediaFormat_getInt32(trackFormat, AMEDIAFORMAT_KEY_HEIGHT, &height);
                m_info.width = static_cast<uint32_t>(width);
                m_info.height = static_cast<uint32_t>(height);
                m_impl->frameWidth = m_info.width;
                m_impl->frameHeight = m_info.height;

                // Frame rate
                int32_t frameRate = 30;
                AMediaFormat_getInt32(trackFormat, AMEDIAFORMAT_KEY_FRAME_RATE, &frameRate);
                m_info.frameRate = static_cast<double>(frameRate);

                // Duration (microseconds)
                int64_t duration = 0;
                AMediaFormat_getInt64(trackFormat, AMEDIAFORMAT_KEY_DURATION, &duration);
                m_info.duration = static_cast<double>(duration) / 1000000.0;

                // Codec
                if (strcmp(mime, "video/avc") == 0)
                    m_info.codec = "H.264";
                else if (strcmp(mime, "video/hevc") == 0)
                    m_info.codec = "HEVC";
                else if (strcmp(mime, "video/x-vnd.on2.vp9") == 0)
                    m_info.codec = "VP9";
                else
                    m_info.codec = mime;

                LOGD("Video track found: %dx%d, %.1f fps, %.2f sec, codec=%s",
                     width, height, m_info.frameRate, m_info.duration, mime);
                break;
            }
            else
            {
                AMediaFormat_delete(trackFormat);
            }
        }

        if (m_impl->videoTrackIndex < 0)
        {
            setError("No video track found");
            close();
            return false;
        }

        // Select video track
        AMediaExtractor_selectTrack(m_impl->extractor, m_impl->videoTrackIndex);

        // Check for audio track
        for (int i = 0; i < numTracks; i++)
        {
            if (i == m_impl->videoTrackIndex) continue;
            AMediaFormat* trackFormat = AMediaExtractor_getTrackFormat(m_impl->extractor, i);
            const char* mime = nullptr;
            AMediaFormat_getString(trackFormat, AMEDIAFORMAT_KEY_MIME, &mime);
            if (mime && strncmp(mime, "audio/", 6) == 0)
            {
                m_info.hasAudio = true;
                m_impl->audioTrackIndex = i;
            }
            AMediaFormat_delete(trackFormat);
            if (m_info.hasAudio) break;
        }

        // Create decoder
        const char* mime = nullptr;
        AMediaFormat_getString(m_impl->format, AMEDIAFORMAT_KEY_MIME, &mime);
        m_impl->codec = AMediaCodec_createDecoderByType(mime);
        if (!m_impl->codec)
        {
            setError("Failed to create decoder for: " + std::string(mime));
            close();
            return false;
        }

        // Configure decoder
        media_status_t status = AMediaCodec_configure(m_impl->codec, m_impl->format, nullptr, nullptr, 0);
        if (status != AMEDIA_OK)
        {
            setError("Failed to configure decoder");
            close();
            return false;
        }

        // Start decoder
        status = AMediaCodec_start(m_impl->codec);
        if (status != AMEDIA_OK)
        {
            setError("Failed to start decoder");
            close();
            return false;
        }

        // Get output format to determine color format, stride, and slice height
        AMediaFormat* outputFormat = AMediaCodec_getOutputFormat(m_impl->codec);
        if (outputFormat)
        {
            int32_t colorFormat = 0;
            AMediaFormat_getInt32(outputFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat);
            m_impl->colorFormat = colorFormat;

            int32_t stride = m_impl->frameWidth;
            AMediaFormat_getInt32(outputFormat, AMEDIAFORMAT_KEY_STRIDE, &stride);
            m_impl->frameStride = static_cast<uint32_t>(stride > 0 ? stride : m_impl->frameWidth);

            // Get slice height (may be 0 or missing on some devices)
            // Slice height is the vertical stride - where UV plane starts
            int32_t sliceHeight = 0;
            AMediaFormat_getInt32(outputFormat, "slice-height", &sliceHeight);
            if (sliceHeight <= 0)
            {
                // Fallback: align to 16 pixels (common hardware alignment)
                sliceHeight = (m_impl->frameHeight + 15) & ~15;
            }
            m_impl->sliceHeight = static_cast<uint32_t>(sliceHeight);

            LOGD("Output format: stride=%d, sliceHeight=%d, colorFormat=%d",
                 m_impl->frameStride, m_impl->sliceHeight, colorFormat);

            AMediaFormat_delete(outputFormat);
        }
        else
        {
            m_impl->frameStride = m_impl->frameWidth;
            m_impl->sliceHeight = (m_impl->frameHeight + 15) & ~15;
        }

        // Allocate frame buffer (YUV420 = 1.5 bytes per pixel)
        // Use sliceHeight for proper UV plane placement
        size_t ySize = static_cast<size_t>(m_impl->frameStride) * m_impl->sliceHeight;
        size_t uvSize = static_cast<size_t>(m_impl->frameStride) * (m_impl->sliceHeight / 2);
        m_impl->frameBuffer.resize(ySize + uvSize);

        m_state = DecoderState::Ready;
        m_currentTime = 0.0;

        return true;
    }

    void VideoDecoder::close()
    {
        if (m_impl)
            m_impl->release();

        m_info = VideoInfo{};
        m_state = DecoderState::Closed;
        m_currentTime = 0.0;
        m_lastError.clear();
    }

    bool VideoDecoder::decodeFrame(VideoFrame& outFrame)
    {
        if (m_state != DecoderState::Ready || !m_impl->codec || !m_impl->extractor)
            return false;

        const int64_t TIMEOUT_US = 10000; // 10ms timeout

        // Feed input to decoder
        if (!m_impl->inputEOS)
        {
            ssize_t bufferIndex = AMediaCodec_dequeueInputBuffer(m_impl->codec, TIMEOUT_US);
            if (bufferIndex >= 0)
            {
                size_t bufferSize = 0;
                uint8_t* buffer = AMediaCodec_getInputBuffer(m_impl->codec, bufferIndex, &bufferSize);

                ssize_t sampleSize = AMediaExtractor_readSampleData(m_impl->extractor, buffer, bufferSize);
                int64_t presentationTimeUs = AMediaExtractor_getSampleTime(m_impl->extractor);

                if (sampleSize < 0)
                {
                    // End of stream
                    sampleSize = 0;
                    m_impl->inputEOS = true;
                    AMediaCodec_queueInputBuffer(m_impl->codec, bufferIndex, 0, 0,
                        presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                }
                else
                {
                    AMediaCodec_queueInputBuffer(m_impl->codec, bufferIndex, 0,
                        static_cast<size_t>(sampleSize), presentationTimeUs, 0);
                    AMediaExtractor_advance(m_impl->extractor);
                }
            }
        }

        // Get output from decoder
        AMediaCodecBufferInfo info;
        ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(m_impl->codec, &info, TIMEOUT_US);

        if (outputIndex >= 0)
        {
            if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
            {
                m_impl->outputEOS = true;
                m_state = DecoderState::EndOfStream;
                AMediaCodec_releaseOutputBuffer(m_impl->codec, outputIndex, false);
                return false;
            }

            // Get output buffer
            size_t bufferSize = 0;
            uint8_t* buffer = AMediaCodec_getOutputBuffer(m_impl->codec, outputIndex, &bufferSize);

            if (buffer && info.size > 0)
            {
                const uint8_t* srcData = buffer + info.offset;
                const uint32_t width = m_impl->frameWidth;
                const uint32_t height = m_impl->frameHeight;
                const uint32_t srcStride = m_impl->frameStride;
                const uint32_t sliceHeight = m_impl->sliceHeight;

                // Copy Y plane row-by-row (handles stride mismatch)
                uint8_t* yDst = m_impl->frameBuffer.data();
                for (uint32_t y = 0; y < height; ++y)
                {
                    memcpy(yDst + y * srcStride, srcData + y * srcStride, width);
                }

                // UV plane starts at sliceHeight * stride in source buffer
                // MediaCodec typically outputs NV12 (COLOR_FormatYUV420SemiPlanar = 21)
                // or I420 (COLOR_FormatYUV420Planar = 19)
                const uint8_t* uvSrc = srcData + static_cast<size_t>(sliceHeight) * srcStride;
                uint8_t* uvDst = m_impl->frameBuffer.data() + static_cast<size_t>(sliceHeight) * srcStride;
                const uint32_t chromaHeight = height / 2;

                if (m_impl->colorFormat == 21) // NV12 - interleaved UV
                {
                    for (uint32_t y = 0; y < chromaHeight; ++y)
                    {
                        memcpy(uvDst + y * srcStride, uvSrc + y * srcStride, width);
                    }
                }
                else // I420/YUV420P - planar U and V
                {
                    // For planar formats, U and V planes are separate
                    // Each has half the stride and half the height
                    const uint32_t uvStride = srcStride / 2;
                    const size_t uPlaneSize = static_cast<size_t>(uvStride) * chromaHeight;

                    for (uint32_t y = 0; y < chromaHeight; ++y)
                    {
                        // U plane
                        memcpy(uvDst + y * uvStride, uvSrc + y * uvStride, width / 2);
                    }
                    // V plane follows U plane
                    const uint8_t* vSrc = uvSrc + uPlaneSize;
                    uint8_t* vDst = uvDst + uPlaneSize;
                    for (uint32_t y = 0; y < chromaHeight; ++y)
                    {
                        memcpy(vDst + y * uvStride, vSrc + y * uvStride, width / 2);
                    }
                }
            }

            // Release buffer
            AMediaCodec_releaseOutputBuffer(m_impl->codec, outputIndex, false);

            // Update timestamp
            m_currentTime = static_cast<double>(info.presentationTimeUs) / 1000000.0;

            // Fill output frame pointers
            // UV plane starts at sliceHeight * stride (not frameHeight * stride)
            if (m_impl->colorFormat == 21) // NV12
            {
                outFrame.format = PixelFormat::NV12;
                outFrame.planes[0] = m_impl->frameBuffer.data();
                outFrame.planes[1] = m_impl->frameBuffer.data() +
                    (static_cast<size_t>(m_impl->frameStride) * m_impl->sliceHeight);
                outFrame.planes[2] = nullptr;
                outFrame.linesize[0] = m_impl->frameStride;
                outFrame.linesize[1] = m_impl->frameStride;
                outFrame.linesize[2] = 0;
            }
            else // Assume I420/YUV420P
            {
                outFrame.format = PixelFormat::YUV420P;
                size_t ySize = static_cast<size_t>(m_impl->frameStride) * m_impl->sliceHeight;
                size_t uvStride = m_impl->frameStride / 2;
                size_t uvHeight = m_impl->sliceHeight / 2;

                outFrame.planes[0] = m_impl->frameBuffer.data();
                outFrame.planes[1] = m_impl->frameBuffer.data() + ySize;
                outFrame.planes[2] = m_impl->frameBuffer.data() + ySize + (uvStride * uvHeight);
                outFrame.linesize[0] = m_impl->frameStride;
                outFrame.linesize[1] = uvStride;
                outFrame.linesize[2] = uvStride;
            }

            outFrame.width = m_impl->frameWidth;
            outFrame.height = m_impl->frameHeight;
            outFrame.pts = m_currentTime;
            outFrame.duration = 1.0 / m_info.frameRate;

            return true;
        }
        else if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
        {
            // Format changed, get new format info
            AMediaFormat* newFormat = AMediaCodec_getOutputFormat(m_impl->codec);
            if (newFormat)
            {
                int32_t width = 0, height = 0, stride = 0, sliceHeight = 0, colorFormat = 0;
                AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_WIDTH, &width);
                AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_HEIGHT, &height);
                AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_STRIDE, &stride);
                AMediaFormat_getInt32(newFormat, "slice-height", &sliceHeight);
                AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_COLOR_FORMAT, &colorFormat);

                m_impl->frameWidth = static_cast<uint32_t>(width);
                m_impl->frameHeight = static_cast<uint32_t>(height);
                m_impl->frameStride = stride > 0 ? static_cast<uint32_t>(stride) : m_impl->frameWidth;
                m_impl->colorFormat = colorFormat;

                // Handle sliceHeight (may be 0 on some devices)
                if (sliceHeight <= 0)
                    sliceHeight = (height + 15) & ~15;  // Align to 16
                m_impl->sliceHeight = static_cast<uint32_t>(sliceHeight);

                // Reallocate buffer with proper size for aligned height
                size_t ySize = static_cast<size_t>(m_impl->frameStride) * m_impl->sliceHeight;
                size_t uvSize = static_cast<size_t>(m_impl->frameStride) * (m_impl->sliceHeight / 2);
                size_t bufferSize = ySize + uvSize;
                if (m_impl->frameBuffer.size() < bufferSize)
                    m_impl->frameBuffer.resize(bufferSize);

                LOGD("Format changed: %dx%d, stride=%d, sliceHeight=%d, colorFormat=%d",
                     width, height, stride, sliceHeight, colorFormat);

                AMediaFormat_delete(newFormat);
            }
            // Try again
            return decodeFrame(outFrame);
        }
        else if (outputIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER)
        {
            // Need more input, try again
            if (m_impl->inputEOS && m_impl->outputEOS)
            {
                m_state = DecoderState::EndOfStream;
                return false;
            }
            return decodeFrame(outFrame);
        }

        return false;
    }

    bool VideoDecoder::seek(double timestamp)
    {
        // Allow seeking in Ready or EndOfStream states
        if ((m_state != DecoderState::Ready && m_state != DecoderState::EndOfStream) ||
            !m_impl->extractor || !m_impl->codec)
            return false;

        // Convert to microseconds
        int64_t positionUs = static_cast<int64_t>(timestamp * 1000000.0);

        // Seek extractor
        media_status_t status = AMediaExtractor_seekTo(
            m_impl->extractor, positionUs, AMEDIAEXTRACTOR_SEEK_PREVIOUS_SYNC);

        if (status != AMEDIA_OK)
        {
            setError("Seek failed");
            return false;
        }

        // Flush codec
        AMediaCodec_flush(m_impl->codec);

        m_impl->inputEOS = false;
        m_impl->outputEOS = false;
        m_currentTime = timestamp;

        // Reset to ready state (handles EndOfStream case)
        m_state = DecoderState::Ready;

        return true;
    }

    bool VideoDecoder::decodeAllAudio(AudioData& outAudio)
    {
        if (m_state != DecoderState::Ready || !m_info.hasAudio || m_impl->audioTrackIndex < 0)
            return false;

        outAudio = AudioData{};

        // Create a separate extractor for audio to avoid disturbing video state
        AMediaExtractor* audioExtractor = AMediaExtractor_new();
        if (!audioExtractor)
        {
            LOGE("Failed to create audio extractor");
            return false;
        }

        media_status_t status;
        if (m_impl->assetFd >= 0)
        {
            status = AMediaExtractor_setDataSourceFd(
                audioExtractor, m_impl->assetFd, m_impl->assetOffset, m_impl->assetLength);
        }
        else
        {
            status = AMediaExtractor_setDataSource(audioExtractor, m_impl->filePath.c_str());
        }
        if (status != AMEDIA_OK)
        {
            LOGE("Failed to set audio extractor data source");
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        // Select audio track
        AMediaExtractor_selectTrack(audioExtractor, m_impl->audioTrackIndex);

        // Get audio format
        AMediaFormat* audioFormat = AMediaExtractor_getTrackFormat(audioExtractor, m_impl->audioTrackIndex);
        if (!audioFormat)
        {
            LOGE("Failed to get audio format");
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        const char* mime = nullptr;
        AMediaFormat_getString(audioFormat, AMEDIAFORMAT_KEY_MIME, &mime);

        int32_t sampleRate = 0, channelCount = 0;
        AMediaFormat_getInt32(audioFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, &sampleRate);
        AMediaFormat_getInt32(audioFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &channelCount);

        outAudio.sampleRate = static_cast<uint32_t>(sampleRate);
        outAudio.channels = static_cast<uint32_t>(channelCount);

        LOGD("Audio format: %s, %dHz, %d channels", mime ? mime : "unknown", sampleRate, channelCount);

        if (outAudio.sampleRate == 0 || outAudio.channels == 0)
        {
            AMediaFormat_delete(audioFormat);
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        // Create audio decoder
        AMediaCodec* audioCodec = AMediaCodec_createDecoderByType(mime);
        if (!audioCodec)
        {
            LOGE("Failed to create audio decoder for %s", mime ? mime : "unknown");
            AMediaFormat_delete(audioFormat);
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        status = AMediaCodec_configure(audioCodec, audioFormat, nullptr, nullptr, 0);
        AMediaFormat_delete(audioFormat);

        if (status != AMEDIA_OK)
        {
            LOGE("Failed to configure audio decoder");
            AMediaCodec_delete(audioCodec);
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        status = AMediaCodec_start(audioCodec);
        if (status != AMEDIA_OK)
        {
            LOGE("Failed to start audio decoder");
            AMediaCodec_delete(audioCodec);
            AMediaExtractor_delete(audioExtractor);
            return false;
        }

        // Reserve approximate space
        size_t estimatedSamples = static_cast<size_t>(
            m_info.duration * outAudio.sampleRate * outAudio.channels * 1.1);
        outAudio.samples.reserve(estimatedSamples);

        const int64_t TIMEOUT_US = 10000; // 10ms
        bool inputEOS = false;
        bool outputEOS = false;

        while (!outputEOS)
        {
            // Feed input
            if (!inputEOS)
            {
                ssize_t bufferIndex = AMediaCodec_dequeueInputBuffer(audioCodec, TIMEOUT_US);
                if (bufferIndex >= 0)
                {
                    size_t bufferSize = 0;
                    uint8_t* buffer = AMediaCodec_getInputBuffer(audioCodec, bufferIndex, &bufferSize);

                    ssize_t sampleSize = AMediaExtractor_readSampleData(audioExtractor, buffer, bufferSize);
                    int64_t presentationTimeUs = AMediaExtractor_getSampleTime(audioExtractor);

                    if (sampleSize < 0)
                    {
                        inputEOS = true;
                        AMediaCodec_queueInputBuffer(audioCodec, bufferIndex, 0, 0,
                            presentationTimeUs, AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM);
                    }
                    else
                    {
                        AMediaCodec_queueInputBuffer(audioCodec, bufferIndex, 0,
                            static_cast<size_t>(sampleSize), presentationTimeUs, 0);
                        AMediaExtractor_advance(audioExtractor);
                    }
                }
            }

            // Get output
            AMediaCodecBufferInfo info;
            ssize_t outputIndex = AMediaCodec_dequeueOutputBuffer(audioCodec, &info, TIMEOUT_US);

            if (outputIndex >= 0)
            {
                if (info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM)
                {
                    outputEOS = true;
                }
                else if (info.size > 0)
                {
                    size_t bufferSize = 0;
                    uint8_t* buffer = AMediaCodec_getOutputBuffer(audioCodec, outputIndex, &bufferSize);

                    if (buffer)
                    {
                        // Audio decoder outputs PCM 16-bit by default
                        size_t numSamples = static_cast<size_t>(info.size) / sizeof(int16_t);
                        const int16_t* pcmData = reinterpret_cast<const int16_t*>(buffer + info.offset);
                        outAudio.samples.insert(outAudio.samples.end(), pcmData, pcmData + numSamples);
                    }
                }
                AMediaCodec_releaseOutputBuffer(audioCodec, outputIndex, false);
            }
            else if (outputIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED)
            {
                // Format changed, continue
                AMediaFormat* newFormat = AMediaCodec_getOutputFormat(audioCodec);
                if (newFormat)
                {
                    int32_t newSampleRate = 0, newChannels = 0;
                    AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_SAMPLE_RATE, &newSampleRate);
                    AMediaFormat_getInt32(newFormat, AMEDIAFORMAT_KEY_CHANNEL_COUNT, &newChannels);
                    if (newSampleRate > 0) outAudio.sampleRate = static_cast<uint32_t>(newSampleRate);
                    if (newChannels > 0) outAudio.channels = static_cast<uint32_t>(newChannels);
                    LOGD("Audio output format changed: %dHz, %d channels", newSampleRate, newChannels);
                    AMediaFormat_delete(newFormat);
                }
            }
            // AMEDIACODEC_INFO_TRY_AGAIN_LATER - just continue loop
        }

        // Cleanup
        AMediaCodec_stop(audioCodec);
        AMediaCodec_delete(audioCodec);
        AMediaExtractor_delete(audioExtractor);

        // Calculate duration
        if (outAudio.sampleRate > 0 && outAudio.channels > 0)
        {
            outAudio.duration = static_cast<double>(outAudio.samples.size()) /
                (outAudio.sampleRate * outAudio.channels);
        }

        LOGD("Decoded %zu audio samples (%.2f sec)", outAudio.samples.size(), outAudio.duration);

        return !outAudio.samples.empty();
    }

    void VideoDecoder::setError(const std::string& error)
    {
        m_lastError = error;
        m_state = DecoderState::Error;
        LOGE("%s", error.c_str());
    }

} // namespace video

#endif // __ANDROID__
#endif // MAGIC_HAS_VIDEO
