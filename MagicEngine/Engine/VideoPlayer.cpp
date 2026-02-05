/******************************************************************************/
/*!
\file   VideoPlayer.cpp
\brief  Video playback component and VideoManager singleton implementation.
*/
/******************************************************************************/
#include "Engine/VideoPlayer.h"
#include "Assets/AssetManager.h"
#include "VFS/VFS.h"
#include "ECS/ECS.h"
#include "video/video_decoder.h"
#include "Editor/Containers/GUICollection.h"
#include "resource/resource_manager.h"
#include "renderer/gfx_interface.h"
#include "Managers/AudioManager.h"
#include "Game/GameSystems.h"
#include "logging/log.h"
#include <cstring>

// ============================================================================
// VideoPlayerComponent
// ============================================================================

VideoPlayerComponent::VideoPlayerComponent()
    : m_videoFile(0)
    , m_isPlaying(false)
    , m_isPaused(false)
    , m_loop(false)
    , m_playbackSpeed(1.0f)
    , m_currentTime(0.0f)
    , m_seekTarget(-1.0f)
    , m_decoderHandle(0)
{
}

VideoPlayerComponent::~VideoPlayerComponent()
{
    if (m_decoderHandle != 0)
    {
        if (VideoManager* mgr = ST<VideoManager>::Get())
            mgr->DestroyDecoder(m_decoderHandle);
        m_decoderHandle = 0;
    }
}

void VideoPlayerComponent::Play()
{
    LOG_INFO("[VideoPlayer] Play() called, videoFile={}, decoderHandle={}", m_videoFile, m_decoderHandle);

    if (m_videoFile == 0)
    {
        LOG_WARNING("[VideoPlayer] Play() called but no video file set");
        return;
    }

    VideoManager* mgr = ST<VideoManager>::Get();
    if (!mgr)
    {
        LOG_ERROR("[VideoPlayer] VideoManager not found!");
        return;
    }

    // If paused, just resume
    if (m_isPaused)
    {
        m_isPaused = false;
        m_isPlaying = true;
        if (m_decoderHandle != 0)
            mgr->ResumeAudio(m_decoderHandle);
        return;
    }

    // Create decoder if needed
    if (m_decoderHandle == 0 || !mgr->IsValidHandle(m_decoderHandle))
    {
        m_decoderHandle = mgr->CreateDecoder(m_videoFile);
        LOG_INFO("[VideoPlayer] CreateDecoder returned handle: {}", m_decoderHandle);
    }

    if (m_decoderHandle != 0)
    {
        mgr->StartPlayback(m_decoderHandle);
        m_isPlaying = true;
        m_isPaused = false;
        m_currentTime = 0.0f;
        LOG_INFO("[VideoPlayer] Playback started");
    }
    else
    {
        LOG_ERROR("[VideoPlayer] Failed to create decoder, playback not started");
    }
}

void VideoPlayerComponent::Pause()
{
    if (m_isPlaying)
    {
        m_isPaused = true;
        m_isPlaying = false;

        if (m_decoderHandle != 0)
        {
            if (VideoManager* mgr = ST<VideoManager>::Get())
                mgr->PauseAudio(m_decoderHandle);
        }
    }
}

void VideoPlayerComponent::Stop()
{
    m_isPlaying = false;
    m_isPaused = false;
    m_currentTime = 0.0f;

    if (m_decoderHandle != 0)
    {
        if (VideoManager* mgr = ST<VideoManager>::Get())
            mgr->StopAndReset(m_decoderHandle);
    }
}

void VideoPlayerComponent::Seek(float timestamp)
{
    m_seekTarget = timestamp;
}

float VideoPlayerComponent::GetVideoDuration() const
{
    ResourceVideo* video = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceVideo>().INTERNAL_GetResource(m_videoFile, false);
    if (video && video->EnsureMetadataLoaded())
        return video->data.duration;
    return 0.0f;
}

bool VideoPlayerComponent::HasFinished() const
{
    if (m_isPlaying || m_isPaused)
        return false;

    float duration = GetVideoDuration();
    if (duration <= 0.0f)
        return false;

    return m_currentTime >= duration - 0.001f;
}

void VideoPlayerComponent::SetVideoFile(size_t assetHash)
{
    if (m_videoFile == assetHash)
        return;

    Stop();

    // Destroy old decoder before switching
    if (m_decoderHandle != 0)
    {
        if (VideoManager* mgr = ST<VideoManager>::Get())
            mgr->DestroyDecoder(m_decoderHandle);
        m_decoderHandle = 0;
    }

    m_videoFile = assetHash;

    if (m_videoFile != 0)
    {
        if (VideoManager* mgr = ST<VideoManager>::Get())
        {
            m_decoderHandle = mgr->CreateDecoder(m_videoFile);
            if (m_decoderHandle != 0)
                mgr->DecodeFirstFrame(m_decoderHandle);
        }
    }
}

void VideoPlayerComponent::EditorDraw()
{
#ifdef IMGUI_ENABLED
    if (m_videoFile != 0)
    {
        const std::string* name = ST<AssetManager>::Get()->Editor_GetName(m_videoFile);
        gui::TextBoxReadOnly("Video File", name ? *name : "Unknown");
    }
    else
    {
        gui::TextBoxReadOnly("Video File", "(None)");
    }
    gui::PayloadTarget<size_t>("VIDEO_HASH", [this](size_t hash) {
        SetVideoFile(hash);
    });

    ResourceVideo* video = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceVideo>().INTERNAL_GetResource(m_videoFile, false);
    if (video && video->EnsureMetadataLoaded())
    {
        ImGui::Text("Resolution: %u x %u", video->data.width, video->data.height);
        ImGui::Text("Duration: %.2f sec", video->data.duration);
        ImGui::Text("Frame Rate: %.2f fps", video->data.frameRate);
    }

    ImGui::Separator();

    gui::Checkbox("Auto Play", &m_autoPlay);
    gui::Checkbox("Loop", &m_loop);
    gui::Checkbox("Fullscreen", &m_fullscreen);
    if (!m_fullscreen)
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(Position/Scale from RectTransform)");
    gui::VarDrag("Playback Speed", &m_playbackSpeed, 0.1f, 0.1f, 4.0f);
    gui::VarDrag("Audio Volume", &m_audioVolume, 0.01f, 0.0f, 1.0f);
    if (m_audioVolume < 0.0001f)
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(Audio muted)");
    // Note: Layer is now controlled by RectTransformComponent (shown above in inspector)

    ImGui::Separator();

    if (m_isPlaying)
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1.0f), "Status: Playing");
    else if (m_isPaused)
        ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.2f, 1.0f), "Status: Paused");
    else
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Status: Stopped");

    float duration = GetVideoDuration();
    if (duration > 0)
    {
        ImGui::Text("Time: %.2f / %.2f", m_currentTime, duration);
        ImGui::ProgressBar(m_currentTime / duration, ImVec2(-1, 0));
    }

    ImGui::BeginDisabled(m_videoFile == 0);
    if (!m_isPlaying)
    {
        if (gui::Button("Play", gui::Vec2{ 60.0f, 25.0f }))
            Play();
    }
    else
    {
        if (gui::Button("Pause", gui::Vec2{ 60.0f, 25.0f }))
            Pause();
    }
    gui::SameLine();
    if (gui::Button("Stop", gui::Vec2{ 60.0f, 25.0f }))
        Stop();
    ImGui::EndDisabled();
#endif
}

// ============================================================================
// VideoManager (Singleton)
// ============================================================================

static constexpr int BUFFER_COUNT = 3;
static constexpr int FRAME_DELAY = 2;

struct FrameBuffer
{
    hina_buffer yStaging = {};
    hina_buffer uvStaging = {};
    hina_texture yTexture = {};
    hina_texture uvTexture = {};
    hina_texture_view yView = {};
    hina_texture_view uvView = {};
    hina_texture rgbaTexture = {};
    hina_texture_view rgbaView = {};
    uint64_t frameUploaded = 0;
    bool hasData = false;
    bool needsConversion = false;
};

static void CopyYPlane(const video::VideoFrame& frame, void* dst, uint32_t width, uint32_t height)
{
    const uint8_t* src = frame.planes[0];
    const uint32_t srcStride = frame.linesize[0];
    uint8_t* dstPtr = static_cast<uint8_t*>(dst);
    for (uint32_t y = 0; y < height; ++y)
        std::memcpy(dstPtr + y * width, src + y * srcStride, width);
}

static void CopyUVPlane(const video::VideoFrame& frame, void* dst, uint32_t width, uint32_t height)
{
    uint8_t* dstPtr = static_cast<uint8_t*>(dst);
    const uint32_t chromaWidth = width / 2;
    const uint32_t chromaHeight = height / 2;

    if (frame.format == video::PixelFormat::NV12)
    {
        const uint8_t* src = frame.planes[1];
        const uint32_t srcStride = frame.linesize[1];
        for (uint32_t y = 0; y < chromaHeight; ++y)
            std::memcpy(dstPtr + y * chromaWidth * 2, src + y * srcStride, chromaWidth * 2);
    }
    else if (frame.format == video::PixelFormat::YUV420P)
    {
        const uint8_t* uSrc = frame.planes[1];
        const uint8_t* vSrc = frame.planes[2];
        const uint32_t uStride = frame.linesize[1];
        const uint32_t vStride = frame.linesize[2];
        for (uint32_t y = 0; y < chromaHeight; ++y)
        {
            uint8_t* dstRow = dstPtr + y * chromaWidth * 2;
            for (uint32_t x = 0; x < chromaWidth; ++x)
            {
                dstRow[x * 2 + 0] = uSrc[y * uStride + x];
                dstRow[x * 2 + 1] = vSrc[y * vStride + x];
            }
        }
    }
}

static const char* kYuvToRgbShader = R"(#hina
group Video = 0;
bindings(Video, start=0) {
  texture sampler2D yTexture;
  texture sampler2D uvTexture;
}
struct Varyings { vec2 uv; };
struct FragOut { vec4 color; };
#hina_end

#hina_stage vertex entry VSMain
Varyings VSMain() {
    Varyings out;
    out.uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
    gl_Position = vec4(out.uv * vec2(2, 2) + vec2(-1, -1), 0.0, 1.0);
    return out;
}
#hina_end

#hina_stage fragment entry FSMain
FragOut FSMain(Varyings in) {
    FragOut out;
    float y = texture(yTexture, in.uv).r;
    vec2 uvCoord = clamp(in.uv, vec2(0.002), vec2(0.998));
    vec2 uv_val = texture(uvTexture, uvCoord).rg;
    float y_scaled = (y - 0.0625) * 1.164;
    float u = uv_val.r - 0.5;
    float v = uv_val.g - 0.5;
    vec3 rgb;
    rgb.r = y_scaled + 1.596 * v;
    rgb.g = y_scaled - 0.391 * u - 0.813 * v;
    rgb.b = y_scaled + 2.018 * u;
    out.color = vec4(clamp(rgb, 0.0, 1.0), 1.0);
    return out;
}
#hina_end
)";

static hina_pipeline s_yuvPipeline = {};
static hina_bind_group_layout s_yuvBindGroupLayout = {};
static hina_sampler s_linearSampler = {};
static bool s_yuvResourcesInitialized = false;

static void EnsureYuvConversionResources()
{
    if (s_yuvResourcesInitialized) return;

    hina_sampler_desc samplerDesc = hina_sampler_desc_default();
    samplerDesc.min_filter = HINA_FILTER_LINEAR;
    samplerDesc.mag_filter = HINA_FILTER_LINEAR;
    samplerDesc.address_u = HINA_ADDRESS_CLAMP_TO_EDGE;
    samplerDesc.address_v = HINA_ADDRESS_CLAMP_TO_EDGE;
    s_linearSampler = hina_make_sampler(&samplerDesc);

    char* error = nullptr;
    hina_hsl_module* module = hslc_compile_hsl_source(kYuvToRgbShader, "yuv_to_rgb", &error);
    if (!module)
    {
        LOG_ERROR("[VideoManager] YUV shader compilation failed: {}", error ? error : "Unknown");
        if (error) hslc_free_log(error);
        return;
    }

    hina_hsl_pipeline_desc pipDesc = hina_hsl_pipeline_desc_default();
    pipDesc.cull_mode = HINA_CULL_MODE_NONE;
    pipDesc.primitive_topology = HINA_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipDesc.polygon_mode = HINA_POLYGON_MODE_FILL;
    pipDesc.samples = HINA_SAMPLE_COUNT_1_BIT;
    pipDesc.color_formats[0] = HINA_FORMAT_R8G8B8A8_UNORM;
    pipDesc.depth.depth_test = false;
    pipDesc.depth.depth_write = false;
    pipDesc.depth_format = HINA_FORMAT_UNDEFINED;

    s_yuvPipeline = hina_make_pipeline_from_module(module, &pipDesc, nullptr);
    s_yuvBindGroupLayout = hina_pipeline_get_bind_group_layout(s_yuvPipeline, 0);
    hslc_hsl_module_free(module);

    if (!hina_pipeline_is_valid(s_yuvPipeline))
    {
        LOG_ERROR("[VideoManager] YUV pipeline creation failed");
        return;
    }

    s_yuvResourcesInitialized = true;
    LOG_INFO("[VideoManager] YUV conversion pipeline initialized");
}

struct VideoManager::DecoderInstance
{
    std::unique_ptr<video::VideoDecoder> decoder;
    video::VideoFrame currentFrame;
    bool hasNewFrame = false;
    FrameBuffer frameBuffers[BUFFER_COUNT];
    int writeIndex = 0;
    uint64_t currentFrameNumber = 0;
    double playbackClock = 0.0;
    double lastDecodedPTS = 0.0;
    double lastDecodedDuration = 0.0;
    uint32_t textureWidth = 0;
    uint32_t textureHeight = 0;
    bool resourcesCreated = false;
    video::AudioData audioData;
    FMOD::Sound* audioSound = nullptr;
    FMOD::Channel* audioChannel = nullptr;
    bool audioStarted = false;
};

VideoManager::VideoManager() : m_nextHandle(1) {}

VideoManager::~VideoManager()
{
    // Clean up all decoders
    for (auto& [handle, instance] : m_decoders)
    {
        if (instance->audioChannel)
            instance->audioChannel->stop();
        if (instance->audioSound)
            instance->audioSound->release();

        if (instance->resourcesCreated)
        {
            for (int i = 0; i < BUFFER_COUNT; ++i)
            {
                FrameBuffer& fb = instance->frameBuffers[i];
                if (hina_texture_view_is_valid(fb.yView)) hina_destroy_texture_view(fb.yView);
                if (hina_texture_view_is_valid(fb.uvView)) hina_destroy_texture_view(fb.uvView);
                if (hina_texture_view_is_valid(fb.rgbaView)) hina_destroy_texture_view(fb.rgbaView);
                if (hina_texture_is_valid(fb.yTexture)) hina_destroy_texture(fb.yTexture);
                if (hina_texture_is_valid(fb.uvTexture)) hina_destroy_texture(fb.uvTexture);
                if (hina_texture_is_valid(fb.rgbaTexture)) hina_destroy_texture(fb.rgbaTexture);
                if (hina_buffer_is_valid(fb.yStaging)) hina_destroy_buffer(fb.yStaging);
                if (hina_buffer_is_valid(fb.uvStaging)) hina_destroy_buffer(fb.uvStaging);
            }
        }
    }
    m_decoders.clear();
}

bool VideoManager::IsValidHandle(uint32_t handle) const
{
    return handle != 0 && m_decoders.find(handle) != m_decoders.end();
}

uint32_t VideoManager::CreateDecoder(size_t videoFileHash)
{
    LOG_INFO("[VideoManager] CreateDecoder called with hash: {}", videoFileHash);

    const auto* fileEntry = ST<AssetManager>::Get()->INTERNAL_GetFilepathsManager().GetFileEntry(videoFileHash);
    if (!fileEntry)
    {
        LOG_ERROR("[VideoManager] No file entry found for hash: {}", videoFileHash);
        return 0;
    }

    std::string fullPath = VFS::ConvertVirtualToPhysical(fileEntry->path);
    LOG_INFO("[VideoManager] Opening video file: {}", fullPath);

    auto instance = std::make_unique<DecoderInstance>();
    instance->decoder = std::make_unique<video::VideoDecoder>();

    if (!instance->decoder->open(fullPath))
    {
        LOG_ERROR("[VideoManager] Failed to open video: {}", instance->decoder->getLastError());
        return 0;
    }

    const auto& info = instance->decoder->getInfo();
    if (info.hasAudio)
    {
        if (instance->decoder->decodeAllAudio(instance->audioData))
        {
            LOG_INFO("[VideoManager] Decoded audio: {} samples, {}Hz, {} channels",
                instance->audioData.samples.size(), instance->audioData.sampleRate, instance->audioData.channels);

            AudioManager* audioMgr = ST<AudioManager>::Get();
            if (audioMgr && !instance->audioData.samples.empty())
            {
                FMOD_CREATESOUNDEXINFO exinfo = {};
                exinfo.cbsize = sizeof(FMOD_CREATESOUNDEXINFO);
                exinfo.numchannels = static_cast<int>(instance->audioData.channels);
                exinfo.defaultfrequency = static_cast<int>(instance->audioData.sampleRate);
                exinfo.format = FMOD_SOUND_FORMAT_PCM16;
                exinfo.length = static_cast<unsigned int>(instance->audioData.samples.size() * sizeof(int16_t));

                FMOD::System* fmodSystem = audioMgr->INTERNAL_GetSystem();
                if (fmodSystem)
                {
                    FMOD_RESULT result = fmodSystem->createSound(
                        reinterpret_cast<const char*>(instance->audioData.samples.data()),
                        FMOD_OPENMEMORY | FMOD_OPENRAW | FMOD_LOOP_OFF,
                        &exinfo,
                        &instance->audioSound
                    );
                    if (result != FMOD_OK)
                    {
                        LOG_WARNING("[VideoManager] Failed to create FMOD sound: {}", static_cast<int>(result));
                        instance->audioSound = nullptr;
                    }
                }
            }
        }
    }

    uint32_t handle = m_nextHandle++;
    m_decoders[handle] = std::move(instance);
    return handle;
}

void VideoManager::DestroyDecoder(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;

    DecoderInstance& instance = *it->second;

    if (instance.audioChannel)
    {
        instance.audioChannel->stop();
        instance.audioChannel = nullptr;
    }
    if (instance.audioSound)
    {
        instance.audioSound->release();
        instance.audioSound = nullptr;
    }

    if (instance.resourcesCreated)
    {
        hina_cmd* cmd = hina_cmd_begin();
        hina_ticket ticket = hina_submit_immediate(cmd);
        hina_wait_ticket(ticket);

        for (int i = 0; i < BUFFER_COUNT; ++i)
        {
            FrameBuffer& fb = instance.frameBuffers[i];
            if (hina_texture_view_is_valid(fb.yView)) hina_destroy_texture_view(fb.yView);
            if (hina_texture_view_is_valid(fb.uvView)) hina_destroy_texture_view(fb.uvView);
            if (hina_texture_view_is_valid(fb.rgbaView)) hina_destroy_texture_view(fb.rgbaView);
            if (hina_texture_is_valid(fb.yTexture)) hina_destroy_texture(fb.yTexture);
            if (hina_texture_is_valid(fb.uvTexture)) hina_destroy_texture(fb.uvTexture);
            if (hina_texture_is_valid(fb.rgbaTexture)) hina_destroy_texture(fb.rgbaTexture);
            if (hina_buffer_is_valid(fb.yStaging)) hina_destroy_buffer(fb.yStaging);
            if (hina_buffer_is_valid(fb.uvStaging)) hina_destroy_buffer(fb.uvStaging);
        }
    }

    m_decoders.erase(it);
}

void VideoManager::DecodeFirstFrame(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;

    DecoderInstance& instance = *it->second;
    if (!instance.decoder) return;

    instance.decoder->seek(0.0);
    instance.playbackClock = 0.0;
    instance.lastDecodedPTS = 0.0;
    instance.lastDecodedDuration = 0.0;

    if (instance.decoder->decodeFrame(instance.currentFrame))
    {
        instance.lastDecodedPTS = instance.currentFrame.pts;
        instance.lastDecodedDuration = instance.currentFrame.duration;
        instance.hasNewFrame = true;
        LOG_INFO("[VideoManager] First frame decoded for preview");
    }
}

void VideoManager::StopAndReset(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;

    DecoderInstance& instance = *it->second;

    if (instance.audioChannel)
    {
        instance.audioChannel->stop();
        instance.audioChannel = nullptr;
    }
    instance.audioStarted = false;

    DecodeFirstFrame(handle);
}

void VideoManager::StartPlayback(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;

    DecoderInstance& instance = *it->second;

    if (instance.decoder)
        instance.decoder->seek(0.0);

    instance.playbackClock = 0.0;
    instance.lastDecodedPTS = 0.0;
    instance.lastDecodedDuration = 0.0;
    instance.audioStarted = false;
}

void VideoManager::PauseAudio(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;
    if (it->second->audioChannel)
        it->second->audioChannel->setPaused(true);
}

void VideoManager::ResumeAudio(uint32_t handle)
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;
    if (it->second->audioChannel)
        it->second->audioChannel->setPaused(false);
}

const video::VideoFrame* VideoManager::GetCurrentFrame(uint32_t handle) const
{
    auto it = m_decoders.find(handle);
    if (it != m_decoders.end() && it->second->hasNewFrame)
        return &it->second->currentFrame;
    return nullptr;
}

gfx::TextureView VideoManager::GetTextureView(uint32_t handle) const
{
    auto it = m_decoders.find(handle);
    if (it == m_decoders.end() || !it->second->resourcesCreated)
        return {};

    const DecoderInstance& instance = *it->second;
    const uint64_t currentFrame = instance.currentFrameNumber;

    int bestIndex = -1;
    uint64_t bestFrame = 0;

    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        const FrameBuffer& fb = instance.frameBuffers[i];
        if (!fb.hasData || fb.needsConversion) continue;
        if (currentFrame >= fb.frameUploaded + FRAME_DELAY)
        {
            if (fb.frameUploaded > bestFrame)
            {
                bestFrame = fb.frameUploaded;
                bestIndex = i;
            }
        }
    }

    if (bestIndex >= 0)
        return instance.frameBuffers[bestIndex].rgbaView;
    return {};
}

bool VideoManager::GetVideoDimensions(uint32_t handle, uint32_t& width, uint32_t& height) const
{
    auto it = m_decoders.find(handle);
    if (it != m_decoders.end() && it->second->decoder)
    {
        const auto& info = it->second->decoder->getInfo();
        width = info.width;
        height = info.height;
        return info.width > 0 && info.height > 0;
    }
    return false;
}

void VideoManager::UpdateComponent(VideoPlayerComponent& player, float dt)
{
    uint32_t handle = player.GetDecoderHandle();
    if (handle == 0) return;

    auto it = m_decoders.find(handle);
    if (it == m_decoders.end()) return;

    DecoderInstance& instance = *it->second;

    // Start audio on first update
    float audioVolume = player.GetAudioVolume();
    if (!instance.audioStarted && instance.audioSound)
    {
        AudioManager* audioMgr = ST<AudioManager>::Get();
        if (audioMgr)
        {
            FMOD::System* fmodSystem = audioMgr->INTERNAL_GetSystem();
            if (fmodSystem)
            {
                FMOD_RESULT result = fmodSystem->playSound(instance.audioSound, nullptr, false, &instance.audioChannel);
                if (result == FMOD_OK && instance.audioChannel)
                {
                    float freq = static_cast<float>(instance.audioData.sampleRate) * player.GetPlaybackSpeed();
                    instance.audioChannel->setFrequency(freq);
                    instance.audioChannel->setVolume(audioVolume);
                }
            }
        }
        instance.audioStarted = true;
    }

    if (instance.audioChannel)
    {
        float freq = static_cast<float>(instance.audioData.sampleRate) * player.GetPlaybackSpeed();
        instance.audioChannel->setFrequency(freq);
        instance.audioChannel->setVolume(audioVolume);
    }

    // Handle seek
    float seekTarget = player.GetSeekTarget();
    if (seekTarget >= 0)
    {
        if (instance.decoder->seek(seekTarget))
        {
            player.SetCurrentTime(seekTarget);
            instance.playbackClock = seekTarget;
            instance.lastDecodedPTS = seekTarget;
        }
        player.ClearSeekTarget();
    }

    // Advance clock
    instance.playbackClock += dt * player.GetPlaybackSpeed();
    instance.hasNewFrame = false;

    // Decode frames
    while (true)
    {
        double nextFrameTime = instance.lastDecodedPTS + instance.lastDecodedDuration;
        if (instance.playbackClock < nextFrameTime && instance.lastDecodedDuration > 0)
            break;

        if (instance.decoder->decodeFrame(instance.currentFrame))
        {
            instance.lastDecodedPTS = instance.currentFrame.pts;
            instance.lastDecodedDuration = instance.currentFrame.duration;
            instance.hasNewFrame = true;
            player.SetCurrentTime(static_cast<float>(instance.currentFrame.pts));
        }
        else
        {
            if (instance.decoder->getState() == video::DecoderState::EndOfStream)
            {
                if (player.GetLoop())
                {
                    instance.decoder->seek(0.0);
                    player.SetCurrentTime(0.0f);
                    instance.playbackClock = 0.0;
                    instance.lastDecodedPTS = 0.0;
                    instance.lastDecodedDuration = 0.0;
                    if (instance.audioChannel)
                        instance.audioChannel->setPosition(0, FMOD_TIMEUNIT_MS);
                }
                else
                {
                    if (instance.audioChannel)
                    {
                        instance.audioChannel->stop();
                        instance.audioChannel = nullptr;
                    }
                    instance.audioStarted = false;
                    player.SetPlayingState(false, false);
                    break;
                }
            }
            else
            {
                if (instance.audioChannel)
                {
                    instance.audioChannel->stop();
                    instance.audioChannel = nullptr;
                }
                instance.audioStarted = false;
                player.Stop();
                break;
            }
        }

        if (instance.playbackClock > instance.lastDecodedPTS + instance.lastDecodedDuration * 2)
            instance.hasNewFrame = false;
    }
}

template<typename T>
static void CreateFrameResources(T& instance, uint32_t width, uint32_t height)
{
    instance.textureWidth = width;
    instance.textureHeight = height;

    const size_t ySize = static_cast<size_t>(width) * height;
    const size_t uvSize = static_cast<size_t>(width / 2) * (height / 2) * 2;

    for (int i = 0; i < BUFFER_COUNT; ++i)
    {
        FrameBuffer& fb = instance.frameBuffers[i];

        hina_buffer_desc yBufDesc = hina_buffer_desc_default();
        yBufDesc.size = ySize;
        yBufDesc.memory = HINA_BUFFER_CPU;
        yBufDesc.usage = HINA_BUFFER_TRANSFER_SRC;
        yBufDesc.label = "VideoY_Staging";
        fb.yStaging = hina_make_buffer(&yBufDesc);

        hina_buffer_desc uvBufDesc = hina_buffer_desc_default();
        uvBufDesc.size = uvSize;
        uvBufDesc.memory = HINA_BUFFER_CPU;
        uvBufDesc.usage = HINA_BUFFER_TRANSFER_SRC;
        uvBufDesc.label = "VideoUV_Staging";
        fb.uvStaging = hina_make_buffer(&uvBufDesc);

        hina_texture_desc yTexDesc = hina_texture_desc_default();
        yTexDesc.width = width;
        yTexDesc.height = height;
        yTexDesc.format = HINA_FORMAT_R8_UNORM;
        yTexDesc.mip_levels = 1;
        yTexDesc.usage = HINA_TEXTURE_SAMPLED_BIT;
        yTexDesc.label = "VideoY_Texture";
        fb.yTexture = hina_make_texture(&yTexDesc);
        fb.yView = hina_texture_get_default_view(fb.yTexture);

        hina_texture_desc uvTexDesc = hina_texture_desc_default();
        uvTexDesc.width = width / 2;
        uvTexDesc.height = height / 2;
        uvTexDesc.format = HINA_FORMAT_R8G8_UNORM;
        uvTexDesc.mip_levels = 1;
        uvTexDesc.usage = HINA_TEXTURE_SAMPLED_BIT;
        uvTexDesc.label = "VideoUV_Texture";
        fb.uvTexture = hina_make_texture(&uvTexDesc);
        fb.uvView = hina_texture_get_default_view(fb.uvTexture);

        hina_texture_desc rgbaTexDesc = hina_texture_desc_default();
        rgbaTexDesc.width = width;
        rgbaTexDesc.height = height;
        rgbaTexDesc.format = HINA_FORMAT_R8G8B8A8_UNORM;
        rgbaTexDesc.mip_levels = 1;
        rgbaTexDesc.usage = static_cast<hina_texture_usage_flags>(HINA_TEXTURE_SAMPLED_BIT | HINA_TEXTURE_RENDER_TARGET_BIT);
        rgbaTexDesc.label = "VideoRGBA_Texture";
        fb.rgbaTexture = hina_make_texture(&rgbaTexDesc);
        fb.rgbaView = hina_texture_get_default_view(fb.rgbaTexture);

        fb.frameUploaded = 0;
        fb.hasData = false;
        fb.needsConversion = false;
    }

    instance.resourcesCreated = true;
    LOG_INFO("[VideoManager] Created triple-buffer textures {}x{}", width, height);
}

void VideoManager::UploadPendingFrames(gfx::CommandBuffer& cmdBuf)
{
    for (auto& [handle, instance] : m_decoders)
    {
        instance->currentFrameNumber++;

        if (!instance->hasNewFrame || !instance->currentFrame.isValid())
            continue;

        const video::VideoFrame& frame = instance->currentFrame;
        const uint32_t width = frame.width;
        const uint32_t height = frame.height;

        if (!instance->resourcesCreated || instance->textureWidth != width || instance->textureHeight != height)
        {
            if (instance->resourcesCreated)
            {
                hina_cmd* cmd = hina_cmd_begin();
                hina_ticket ticket = hina_submit_immediate(cmd);
                hina_wait_ticket(ticket);

                for (int i = 0; i < BUFFER_COUNT; ++i)
                {
                    FrameBuffer& fb = instance->frameBuffers[i];
                    if (hina_texture_view_is_valid(fb.yView)) hina_destroy_texture_view(fb.yView);
                    if (hina_texture_view_is_valid(fb.uvView)) hina_destroy_texture_view(fb.uvView);
                    if (hina_texture_view_is_valid(fb.rgbaView)) hina_destroy_texture_view(fb.rgbaView);
                    if (hina_texture_is_valid(fb.yTexture)) hina_destroy_texture(fb.yTexture);
                    if (hina_texture_is_valid(fb.uvTexture)) hina_destroy_texture(fb.uvTexture);
                    if (hina_texture_is_valid(fb.rgbaTexture)) hina_destroy_texture(fb.rgbaTexture);
                    if (hina_buffer_is_valid(fb.yStaging)) hina_destroy_buffer(fb.yStaging);
                    if (hina_buffer_is_valid(fb.uvStaging)) hina_destroy_buffer(fb.uvStaging);
                    fb = {};
                }
                instance->resourcesCreated = false;
            }
            CreateFrameResources(*instance, width, height);
        }

        FrameBuffer& writeBuf = instance->frameBuffers[instance->writeIndex];

        void* yMapped = hina_mapped_buffer_ptr(writeBuf.yStaging);
        if (yMapped) CopyYPlane(frame, yMapped, width, height);

        void* uvMapped = hina_mapped_buffer_ptr(writeBuf.uvStaging);
        if (uvMapped) CopyUVPlane(frame, uvMapped, width, height);

        hina_cmd* cmd = cmdBuf.get();
        hina_cmd_copy_buffer_to_texture(cmd, writeBuf.yStaging, writeBuf.yTexture, 0, 0, 0);
        hina_cmd_copy_buffer_to_texture(cmd, writeBuf.uvStaging, writeBuf.uvTexture, 0, 0, 0);
        hina_cmd_transition_texture(cmd, writeBuf.yTexture, HINA_TEXSTATE_SHADER_READ);
        hina_cmd_transition_texture(cmd, writeBuf.uvTexture, HINA_TEXSTATE_SHADER_READ);

        writeBuf.frameUploaded = instance->currentFrameNumber;
        writeBuf.hasData = true;
        writeBuf.needsConversion = true;

        instance->writeIndex = (instance->writeIndex + 1) % BUFFER_COUNT;
        instance->hasNewFrame = false;
    }
}

void VideoManager::ConvertPendingFrames(gfx::CommandBuffer& cmd)
{
    EnsureYuvConversionResources();
    if (!s_yuvResourcesInitialized) return;

    for (auto& [handle, instance] : m_decoders)
    {
        if (!instance->resourcesCreated) continue;

        for (int i = 0; i < BUFFER_COUNT; ++i)
        {
            FrameBuffer& fb = instance->frameBuffers[i];
            if (!fb.needsConversion) continue;

            hina_cmd_transition_texture(cmd.get(), fb.rgbaTexture, HINA_TEXSTATE_COLOR_ATTACHMENT);

            hina_pass_action passAction = {};
            passAction.colors[0].image = fb.rgbaView;
            passAction.colors[0].load_op = HINA_LOAD_OP_CLEAR;
            passAction.colors[0].store_op = HINA_STORE_OP_STORE;
            passAction.colors[0].clear_color[0] = 0.0f;
            passAction.colors[0].clear_color[1] = 0.0f;
            passAction.colors[0].clear_color[2] = 0.0f;
            passAction.colors[0].clear_color[3] = 1.0f;
            passAction.width = instance->textureWidth;
            passAction.height = instance->textureHeight;

            hina_cmd_begin_pass(cmd.get(), &passAction);

            cmd.bindPipeline(s_yuvPipeline);
            cmd.setViewport({.x = 0.0f, .y = 0.0f,
                .width = static_cast<float>(instance->textureWidth),
                .height = static_cast<float>(instance->textureHeight)});
            cmd.setScissor({.x = 0, .y = 0,
                .width = instance->textureWidth,
                .height = instance->textureHeight});

            hina_transient_bind_group tbg = hina_alloc_transient_bind_group(s_yuvBindGroupLayout);
            hina_transient_write_combined_image(&tbg, 0, fb.yView, s_linearSampler);
            hina_transient_write_combined_image(&tbg, 1, fb.uvView, s_linearSampler);
            hina_cmd_bind_transient_group(cmd.get(), 0, tbg);

            cmd.draw(3, 1, 0, 0);
            hina_cmd_end_pass(cmd.get());

            hina_cmd_transition_texture(cmd.get(), fb.rgbaTexture, HINA_TEXSTATE_SHADER_READ);
            fb.needsConversion = false;
        }
    }
}

// ============================================================================
// VideoPlayerSystem (Thin ECS wrapper)
// ============================================================================

bool VideoPlayerSystem::PreRun()
{
    VideoManager* mgr = ST<VideoManager>::Get();
    if (!mgr) return true;

    float dt = 1.0f / 60.0f;

    GAMESTATE currentState = ST<GameSystemsManager>::Get()->GetState();
    bool enteredGameMode = (currentState == GAMESTATE::IN_GAME && m_prevGameState != static_cast<int>(GAMESTATE::IN_GAME));
    m_prevGameState = static_cast<int>(currentState);

    for (auto it = ecs::GetCompsActiveBegin<VideoPlayerComponent>();
         it != ecs::GetCompsEnd<VideoPlayerComponent>(); ++it)
    {
        VideoPlayerComponent& player = *it;

        // Lazy decoder creation for loaded components
        if (player.GetVideoFile() != 0)
        {
            uint32_t handle = player.GetDecoderHandle();
            if (handle == 0 || !mgr->IsValidHandle(handle))
            {
                handle = mgr->CreateDecoder(player.GetVideoFile());
                player.SetDecoderHandle(handle);
                if (handle != 0)
                    mgr->DecodeFirstFrame(handle);
            }
        }

        // AutoPlay on game mode entry
        if (enteredGameMode && player.GetAutoPlay())
        {
            player.Stop();
            player.Play();
        }

        // Update playing components
        if (player.IsPlaying() && !player.IsPaused())
            mgr->UpdateComponent(player, dt);
    }

    return true;
}
