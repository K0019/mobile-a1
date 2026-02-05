/******************************************************************************/
/*!
\file   video_decoder.cpp
\brief  Windows Media Foundation video decoder implementation.
*/
/******************************************************************************/

#ifdef MAGIC_HAS_VIDEO
#ifdef _WIN32

#include "video_decoder.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <propvarutil.h>

#include <vector>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

namespace video
{
    // Helper to convert UTF-8 to wide string
    static std::wstring utf8ToWide(const std::string& str)
    {
        if (str.empty()) return {};
        int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
        std::wstring result(size - 1, 0);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
        return result;
    }

    // RAII wrapper for COM initialization
    struct ComInitializer
    {
        bool initialized = false;

        ComInitializer()
        {
            HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            initialized = SUCCEEDED(hr) || hr == S_FALSE; // S_FALSE = already initialized
        }

        ~ComInitializer()
        {
            if (initialized)
                CoUninitialize();
        }
    };

    // Platform-specific implementation
    struct VideoDecoder::Impl
    {
        ComInitializer comInit;
        IMFSourceReader* sourceReader = nullptr;

        // Current decoded frame buffer (NV12 format)
        std::vector<uint8_t> frameBuffer;
        uint32_t frameWidth = 0;
        uint32_t frameHeight = 0;
        uint32_t frameStride = 0;

        // For non-16-aligned videos, decoder may pad buffer
        uint32_t alignedHeight = 0;  // Actual buffer height (may be > frameHeight)

        ~Impl()
        {
            release();
        }

        void release()
        {
            if (sourceReader)
            {
                sourceReader->Release();
                sourceReader = nullptr;
            }
            frameBuffer.clear();
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

    bool VideoDecoder::open(const std::string& path)
    {
        close();

        // Initialize Media Foundation
        HRESULT hr = MFStartup(MF_VERSION);
        if (FAILED(hr))
        {
            setError("Failed to initialize Media Foundation");
            return false;
        }

        // Create source reader attributes for hardware acceleration
        IMFAttributes* attributes = nullptr;
        hr = MFCreateAttributes(&attributes, 2);
        if (SUCCEEDED(hr))
        {
            // Enable hardware transforms (DXVA)
            attributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
            // Enable video processing (color conversion)
            attributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        }

        // Create source reader from file
        std::wstring widePath = utf8ToWide(path);
        hr = MFCreateSourceReaderFromURL(widePath.c_str(), attributes, &m_impl->sourceReader);

        if (attributes)
            attributes->Release();

        if (FAILED(hr))
        {
            setError("Failed to open video file: " + path);
            MFShutdown();
            return false;
        }

        // Get native media type to extract video info
        IMFMediaType* nativeType = nullptr;
        hr = m_impl->sourceReader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, &nativeType);

        if (SUCCEEDED(hr))
        {
            // Get dimensions
            UINT32 width = 0, height = 0;
            MFGetAttributeSize(nativeType, MF_MT_FRAME_SIZE, &width, &height);
            m_info.width = width;
            m_info.height = height;
            m_impl->frameWidth = width;
            m_impl->frameHeight = height;

            // Get frame rate
            UINT32 numerator = 0, denominator = 1;
            MFGetAttributeRatio(nativeType, MF_MT_FRAME_RATE, &numerator, &denominator);
            if (denominator > 0)
                m_info.frameRate = static_cast<double>(numerator) / denominator;

            // Get codec info
            GUID subtype;
            if (SUCCEEDED(nativeType->GetGUID(MF_MT_SUBTYPE, &subtype)))
            {
                if (subtype == MFVideoFormat_H264)
                    m_info.codec = "H.264";
                else if (subtype == MFVideoFormat_HEVC)
                    m_info.codec = "HEVC";
                else if (subtype == MFVideoFormat_VP90)
                    m_info.codec = "VP9";
                else
                    m_info.codec = "Unknown";
            }

            nativeType->Release();
        }

        // Get duration
        PROPVARIANT var;
        PropVariantInit(&var);
        hr = m_impl->sourceReader->GetPresentationAttribute(
            MF_SOURCE_READER_MEDIASOURCE, MF_PD_DURATION, &var);
        if (SUCCEEDED(hr))
        {
            // Duration is in 100-nanosecond units
            m_info.duration = static_cast<double>(var.uhVal.QuadPart) / 10000000.0;
            PropVariantClear(&var);
        }

        // Check for audio stream
        IMFMediaType* audioType = nullptr;
        hr = m_impl->sourceReader->GetNativeMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, &audioType);
        m_info.hasAudio = SUCCEEDED(hr);
        if (audioType)
            audioType->Release();

        // Configure output format to NV12 (efficient for GPU upload)
        IMFMediaType* outputType = nullptr;
        hr = MFCreateMediaType(&outputType);
        if (SUCCEEDED(hr))
        {
            outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
            outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_NV12);

            hr = m_impl->sourceReader->SetCurrentMediaType(
                MF_SOURCE_READER_FIRST_VIDEO_STREAM, nullptr, outputType);

            outputType->Release();
        }

        if (FAILED(hr))
        {
            setError("Failed to configure decoder output format");
            close();
            return false;
        }

        // Get actual output stride (can be negative for bottom-up images)
        IMFMediaType* actualType = nullptr;
        hr = m_impl->sourceReader->GetCurrentMediaType(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM, &actualType);
        if (SUCCEEDED(hr))
        {
            // MF_MT_DEFAULT_STRIDE can be negative (bottom-up), stored as UINT32 but interpreted as INT32
            UINT32 strideVal = 0;
            hr = actualType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideVal);
            if (SUCCEEDED(hr))
            {
                // Interpret as signed and take absolute value (we only support top-down)
                INT32 signedStride = static_cast<INT32>(strideVal);
                m_impl->frameStride = static_cast<uint32_t>(signedStride < 0 ? -signedStride : signedStride);
            }
            else
            {
                // Fallback: use MFGetStrideForBitmapInfoHeader for NV12
                LONG calculatedStride = 0;
                hr = MFGetStrideForBitmapInfoHeader(MFVideoFormat_NV12.Data1,
                    m_impl->frameWidth, &calculatedStride);
                if (SUCCEEDED(hr) && calculatedStride != 0)
                    m_impl->frameStride = static_cast<uint32_t>(calculatedStride < 0 ? -calculatedStride : calculatedStride);
                else
                    m_impl->frameStride = m_impl->frameWidth;
            }

            actualType->Release();
        }

        // Hardware decoders often pad height to 16-pixel alignment
        // This affects where the UV plane starts in the buffer
        m_impl->alignedHeight = (m_impl->frameHeight + 15) & ~15u;

        // Allocate frame buffer for NV12 (Y plane + UV plane)
        // Use alignedHeight for proper UV plane placement
        size_t ySize = static_cast<size_t>(m_impl->frameStride) * m_impl->alignedHeight;
        size_t uvSize = static_cast<size_t>(m_impl->frameStride) * (m_impl->alignedHeight / 2);
        m_impl->frameBuffer.resize(ySize + uvSize);

        m_state = DecoderState::Ready;
        m_currentTime = 0.0;

        return true;
    }

    void VideoDecoder::close()
    {
        if (m_impl)
            m_impl->release();

        MFShutdown();

        m_info = VideoInfo{};
        m_state = DecoderState::Closed;
        m_currentTime = 0.0;
        m_lastError.clear();
    }

    bool VideoDecoder::decodeFrame(VideoFrame& outFrame)
    {
        if (m_state != DecoderState::Ready || !m_impl->sourceReader)
            return false;

        DWORD streamIndex = 0;
        DWORD flags = 0;
        LONGLONG timestamp = 0;
        IMFSample* sample = nullptr;

        HRESULT hr = m_impl->sourceReader->ReadSample(
            MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,                  // No flags
            &streamIndex,
            &flags,
            &timestamp,
            &sample
        );

        if (FAILED(hr))
        {
            setError("Failed to read video sample");
            return false;
        }

        // Check for end of stream
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
        {
            m_state = DecoderState::EndOfStream;
            return false;
        }

        // Check for format change (rare but possible)
        if (flags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED)
        {
            // Re-query format - for now just continue
        }

        if (!sample)
        {
            // No sample this time, try again
            return decodeFrame(outFrame);
        }

        // Get buffer from sample
        IMFMediaBuffer* buffer = nullptr;
        hr = sample->ConvertToContiguousBuffer(&buffer);

        if (FAILED(hr))
        {
            sample->Release();
            setError("Failed to get sample buffer");
            return false;
        }

        // Try to use IMF2DBuffer for proper stride handling
        // This is the recommended approach for video buffers
        IMF2DBuffer* buffer2D = nullptr;
        hr = buffer->QueryInterface(IID_PPV_ARGS(&buffer2D));

        if (SUCCEEDED(hr) && buffer2D)
        {
            // Use Lock2D for proper pitch-aware access
            BYTE* scanline0 = nullptr;
            LONG pitch = 0;
            hr = buffer2D->Lock2D(&scanline0, &pitch);

            if (SUCCEEDED(hr) && scanline0)
            {
                const uint32_t actualStride = static_cast<uint32_t>(pitch < 0 ? -pitch : pitch);
                const uint32_t width = m_impl->frameWidth;
                const uint32_t height = m_impl->frameHeight;
                const uint32_t alignedHeight = m_impl->alignedHeight;

                // Copy Y plane row by row (handles stride mismatch)
                uint8_t* yDst = m_impl->frameBuffer.data();
                for (uint32_t y = 0; y < height; ++y)
                {
                    memcpy(yDst + y * m_impl->frameStride,
                           scanline0 + y * actualStride,
                           width);
                }

                // UV plane starts at alignedHeight * pitch in source
                // and alignedHeight * frameStride in destination
                const uint8_t* uvSrc = scanline0 + static_cast<size_t>(alignedHeight) * actualStride;
                uint8_t* uvDst = m_impl->frameBuffer.data() + static_cast<size_t>(alignedHeight) * m_impl->frameStride;
                const uint32_t chromaHeight = height / 2;

                for (uint32_t y = 0; y < chromaHeight; ++y)
                {
                    memcpy(uvDst + y * m_impl->frameStride,
                           uvSrc + y * actualStride,
                           width);  // NV12: UV row is same width as Y row
                }

                buffer2D->Unlock2D();
            }
            buffer2D->Release();
        }
        else
        {
            // Fallback: use regular buffer lock (less reliable for stride)
            BYTE* data = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;

            hr = buffer->Lock(&data, &maxLength, &currentLength);
            if (SUCCEEDED(hr) && data)
            {
                size_t copySize = std::min(static_cast<size_t>(currentLength), m_impl->frameBuffer.size());
                memcpy(m_impl->frameBuffer.data(), data, copySize);
                buffer->Unlock();
            }
        }

        buffer->Release();
        sample->Release();

        // Update timestamp (100-nanosecond units to seconds)
        m_currentTime = static_cast<double>(timestamp) / 10000000.0;

        // Fill output frame
        outFrame.format = PixelFormat::NV12;
        outFrame.width = m_impl->frameWidth;
        outFrame.height = m_impl->frameHeight;
        outFrame.pts = m_currentTime;
        outFrame.duration = 1.0 / m_info.frameRate;

        // NV12: Y plane, then interleaved UV plane
        // UV plane starts at alignedHeight * stride (not frameHeight * stride)
        // because hardware decoders pad to 16-pixel alignment
        outFrame.planes[0] = m_impl->frameBuffer.data();
        outFrame.planes[1] = m_impl->frameBuffer.data() +
            (static_cast<size_t>(m_impl->frameStride) * m_impl->alignedHeight);
        outFrame.planes[2] = nullptr;

        outFrame.linesize[0] = m_impl->frameStride;
        outFrame.linesize[1] = m_impl->frameStride;
        outFrame.linesize[2] = 0;

        return true;
    }

    bool VideoDecoder::decodeAllAudio(AudioData& outAudio)
    {
        if (m_state != DecoderState::Ready || !m_info.hasAudio)
            return false;

        outAudio = AudioData{};

        // Create a separate source reader for audio to avoid affecting video decoding
        IMFAttributes* attributes = nullptr;
        HRESULT hr = MFCreateAttributes(&attributes, 1);
        if (FAILED(hr))
            return false;

        // Get the file path from the existing reader (we need to re-open)
        // Actually, we need to track the path. For now, use the same reader
        // and read audio from it. This is simpler but means video must seek back after.

        // Configure audio output format (PCM 16-bit)
        IMFMediaType* audioOutputType = nullptr;
        hr = MFCreateMediaType(&audioOutputType);
        if (FAILED(hr))
        {
            if (attributes) attributes->Release();
            return false;
        }

        audioOutputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        audioOutputType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        audioOutputType->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);

        hr = m_impl->sourceReader->SetCurrentMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioOutputType);
        audioOutputType->Release();

        if (FAILED(hr))
        {
            if (attributes) attributes->Release();
            return false;
        }

        // Get actual audio format info
        IMFMediaType* actualAudioType = nullptr;
        hr = m_impl->sourceReader->GetCurrentMediaType(
            MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualAudioType);

        if (SUCCEEDED(hr))
        {
            UINT32 sampleRate = 0, channels = 0;
            actualAudioType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &sampleRate);
            actualAudioType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);
            outAudio.sampleRate = sampleRate;
            outAudio.channels = channels;
            actualAudioType->Release();
        }

        if (outAudio.sampleRate == 0 || outAudio.channels == 0)
        {
            if (attributes) attributes->Release();
            return false;
        }

        // Reserve approximate space (estimate: duration * sampleRate * channels)
        size_t estimatedSamples = static_cast<size_t>(
            m_info.duration * outAudio.sampleRate * outAudio.channels * 1.1);
        outAudio.samples.reserve(estimatedSamples);

        // Read all audio samples
        while (true)
        {
            DWORD streamIndex = 0;
            DWORD flags = 0;
            LONGLONG timestamp = 0;
            IMFSample* sample = nullptr;

            hr = m_impl->sourceReader->ReadSample(
                MF_SOURCE_READER_FIRST_AUDIO_STREAM,
                0,
                &streamIndex,
                &flags,
                &timestamp,
                &sample
            );

            if (FAILED(hr) || (flags & MF_SOURCE_READERF_ENDOFSTREAM))
            {
                if (sample) sample->Release();
                break;
            }

            if (!sample)
                continue;

            // Get buffer from sample
            IMFMediaBuffer* buffer = nullptr;
            hr = sample->ConvertToContiguousBuffer(&buffer);

            if (SUCCEEDED(hr) && buffer)
            {
                BYTE* data = nullptr;
                DWORD currentLength = 0;

                hr = buffer->Lock(&data, nullptr, &currentLength);
                if (SUCCEEDED(hr) && data && currentLength > 0)
                {
                    // Append PCM samples (16-bit)
                    size_t numSamples = currentLength / sizeof(int16_t);
                    const int16_t* pcmData = reinterpret_cast<const int16_t*>(data);
                    outAudio.samples.insert(outAudio.samples.end(), pcmData, pcmData + numSamples);
                    buffer->Unlock();
                }
                buffer->Release();
            }

            sample->Release();
        }

        if (attributes) attributes->Release();

        // Calculate duration
        if (outAudio.sampleRate > 0 && outAudio.channels > 0)
        {
            outAudio.duration = static_cast<double>(outAudio.samples.size()) /
                (outAudio.sampleRate * outAudio.channels);
        }

        // Reset video position to beginning for normal playback
        seek(0.0);

        return !outAudio.samples.empty();
    }

    bool VideoDecoder::seek(double timestamp)
    {
        // Allow seeking in Ready or EndOfStream states
        if ((m_state != DecoderState::Ready && m_state != DecoderState::EndOfStream) || !m_impl->sourceReader)
            return false;

        // Convert seconds to 100-nanosecond units
        LONGLONG position = static_cast<LONGLONG>(timestamp * 10000000.0);

        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_I8;
        var.hVal.QuadPart = position;

        HRESULT hr = m_impl->sourceReader->SetCurrentPosition(GUID_NULL, var);
        PropVariantClear(&var);

        if (FAILED(hr))
        {
            setError("Seek failed");
            return false;
        }

        m_currentTime = timestamp;

        // Reset to ready state (handles EndOfStream case)
        m_state = DecoderState::Ready;

        return true;
    }

    void VideoDecoder::setError(const std::string& error)
    {
        m_lastError = error;
        m_state = DecoderState::Error;
    }

} // namespace video

#endif // _WIN32
#endif // MAGIC_HAS_VIDEO
