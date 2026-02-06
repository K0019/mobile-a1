/******************************************************************************/
/*!
\file   VideoPlayer.h
\brief  Video playback component for cutscenes and cinematics.

        VideoPlayerComponent provides video playback controls for ECS entities.
        Supports Play/Pause/Stop/Seek operations, controllable via Lua scripts.

        VideoManager is a persistent singleton (like AudioManager) that manages
        decoder instances. VideoPlayerSystem is a thin ECS wrapper that updates
        components each frame.
*/
/******************************************************************************/
#pragma once
#include "Assets/Types/AssetTypesVideo.h"
#include "ECS/IRegisteredComponent.h"
#include "ECS/IEditorComponent.h"
#include "UI/IUIComponent.h"
#include "Game/IGameComponentCallbacks.h"
#include "renderer/gfx_interface.h"
#include <algorithm>
#include <memory>
#include <unordered_map>

// Forward declarations
namespace video { class VideoDecoder; struct VideoFrame; }
namespace Resource { class ResourceManager; }
namespace FMOD { class Sound; class Channel; }

/**
 * @brief Video playback component for cutscenes/cinematics.
 *
 * Attach to an entity to enable video playback. Control via Lua scripts
 * or the editor inspector. This component stores playback configuration;
 * actual decoding is handled by VideoManager.
 */
class VideoPlayerComponent
    : public IRegisteredComponent<VideoPlayerComponent>
    , public IEditorComponent<VideoPlayerComponent>
    , public IUIComponent<VideoPlayerComponent>
    , public virtual IGameComponentCallbacks<VideoPlayerComponent>
{
public:
    VideoPlayerComponent();
    ~VideoPlayerComponent();

    // Playback control (requests to the system)
    void Play();
    void Pause();
    void Stop();
    void Seek(float timestamp);

    // Lua getters/setters
    bool IsPlaying() const { return m_isPlaying; }
    bool IsPaused() const { return m_isPaused; }
    float GetPlaybackTime() const { return m_currentTime; }
    float GetVideoDuration() const;
    float GetPlaybackSpeed() const { return m_playbackSpeed; }
    void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
    size_t GetVideoFile() const { return m_videoFile; }
    void SetVideoFile(size_t assetHash);
    bool GetLoop() const { return m_loop; }
    void SetLoop(bool loop) { m_loop = loop; }

    // Display mode: fullscreen with letterboxing or custom rect
    bool GetFullscreen() const { return m_fullscreen; }
    void SetFullscreen(bool fullscreen) { m_fullscreen = fullscreen; }

    // Auto-play: start playing when ECS enters play state (scene load)
    bool GetAutoPlay() const { return m_autoPlay; }
    void SetAutoPlay(bool autoPlay) { m_autoPlay = autoPlay; }

    // Audio volume (0.0 to 1.0, values < 0.0001 mute audio entirely)
    float GetAudioVolume() const { return m_audioVolume; }
    void SetAudioVolume(float volume) { m_audioVolume = std::max(0.0f, std::min(1.0f, volume)); }

    // Check if video has finished playing (for scene transitions)
    bool HasFinished() const;

    // Internal state (managed by VideoManager)
    uint32_t GetDecoderHandle() const { return m_decoderHandle; }
    void SetDecoderHandle(uint32_t handle) { m_decoderHandle = handle; }
    void SetCurrentTime(float time) { m_currentTime = time; }
    void SetPlayingState(bool playing, bool paused) { m_isPlaying = playing; m_isPaused = paused; }
    float GetSeekTarget() const { return m_seekTarget; }
    void ClearSeekTarget() { m_seekTarget = -1.0f; }

public:
    void OnAttached() override;
    void OnStart() override;

private:
    virtual void EditorDraw() override;

private:
    size_t m_videoFile = 0;                     // Asset hash
    bool m_isPlaying = false;
    bool m_isPaused = false;
    bool m_loop = false;
    float m_playbackSpeed = 1.0f;
    float m_currentTime = 0.0f;
    float m_seekTarget = -1.0f;                 // Pending seek request (-1 = none)
    uint32_t m_decoderHandle = 0;               // Handle to decoder in VideoManager
    bool m_fullscreen = true;                   // Fullscreen with letterboxing
    bool m_autoPlay = false;                    // Start playing when scene loads
    float m_audioVolume = 1.0f;                 // Audio volume (< 0.0001 = muted)

    property_vtable()
};

property_begin(VideoPlayerComponent)
{
    property_var(m_videoFile),
    property_var(m_loop),
    property_var(m_playbackSpeed),
    property_var(m_fullscreen),
    property_var(m_autoPlay),
    property_var(m_audioVolume),
}
property_vend_h(VideoPlayerComponent)

/**
 * @brief Persistent singleton that manages video decoder instances.
 *
 * Like AudioManager, this persists across game state changes.
 * VideoPlayerSystem calls Update() each frame to drive playback.
 */
class VideoManager
{
public:
    // Allow ST to access private members
    friend class ST<VideoManager>;

    VideoManager();
    ~VideoManager();

    // Called each frame by VideoPlayerSystem
    void Update(float dt);

    // Decoder management
    uint32_t CreateDecoder(size_t videoFileHash);
    void DestroyDecoder(uint32_t handle);
    bool IsValidHandle(uint32_t handle) const;

    // Decode first frame for preview
    void DecodeFirstFrame(uint32_t handle);

    // Stop playback: stop audio, seek to beginning, show first frame
    void StopAndReset(uint32_t handle);

    // Start playback: seek to beginning and prepare for audio
    void StartPlayback(uint32_t handle);

    // Pause/resume audio playback
    void PauseAudio(uint32_t handle);
    void ResumeAudio(uint32_t handle);

    // Lifecycle: pause/resume all decoder audio channels
    void PauseAllAudio();
    void ResumeAllAudio();

    // Access decoded frame (for renderer)
    const video::VideoFrame* GetCurrentFrame(uint32_t handle) const;

    // Get RGBA texture view for a video (after YUV->RGB conversion)
    gfx::TextureView GetTextureView(uint32_t handle) const;

    // Upload pending video frames to GPU textures
    void UploadPendingFrames(gfx::CommandBuffer& cmd);

    // Convert YUV textures to RGBA on GPU
    void ConvertPendingFrames(gfx::CommandBuffer& cmd);

    // Get video dimensions (for letterbox calculation)
    bool GetVideoDimensions(uint32_t handle, uint32_t& width, uint32_t& height) const;

    // Update a specific component's playback state
    void UpdateComponent(VideoPlayerComponent& player, float dt);

private:
    struct DecoderInstance;
    std::unordered_map<uint32_t, std::unique_ptr<DecoderInstance>> m_decoders;
    uint32_t m_nextHandle = 1;
};

/**
 * @brief Thin ECS System that updates all video players each frame.
 *
 * Calls into VideoManager singleton for actual work.
 */
class VideoPlayerSystem : public ecs::System<VideoPlayerSystem, VideoPlayerComponent>
{
public:
    VideoPlayerSystem();

private:
    void UpdateComp(VideoPlayerComponent& comp);

};
