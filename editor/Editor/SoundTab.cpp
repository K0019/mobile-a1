#include "Editor/SoundTab.h"
#include "Assets/AssetManager.h"
#include "Editor/Containers/GUICollection.h"
#include "Managers/AudioManager.h"
#include "Editor/SoundGroupWindow.h"

namespace editor {

    const AssetTabConfig SoundTab::config = {
        .name = "Sounds",
        .identifier = ICON_FA_MICROPHONE " Sounds",
        .icon = ICON_FA_MICROPHONE,
        .payloadType = "SOUND_HASH",
        .iconColor = {0.9f, 0.3f, 0.9f, 1.0f},
        .thumbnailType = ThumbnailCache::AssetType::Texture,
        .hasThumbnails = false,
        .detailPanelWidth = 280.0f
    };

    const AssetTabConfig& SoundTab::GetConfig() const
    {
        return config;
    }

    void SoundTab::RenderToolbar()
    {
#ifdef IMGUI_ENABLED
        if (ImGui::Button(ICON_FA_LAYER_GROUP " Sound Groups"))
            editor::CreateGuiWindow<SoundGroupWindow>();
        ImGui::Separator();
#endif
    }

    void SoundTab::RenderContextMenuItems(const size_t& hash)
    {
#ifdef IMGUI_ENABLED
        if (ImGui::MenuItem(ICON_FA_PLAY " Play"))
        {
            PlayPreview(hash);
        }

        // Inherited Delete + SaveToFile
        GenericResourceAssetTab<ResourceAudio>::RenderContextMenuItems(hash);
#endif
    }

    void SoundTab::RenderDetailPanelContent(const size_t& hash, const std::string& name)
    {
#ifdef IMGUI_ENABLED
        // Play / Stop buttons
        bool isPlaying = ST<AudioManager>::Get()->IsPlaying(currentPreviewSound) && lastPreviewAudioHash == hash;

        if (isPlaying)
        {
            if (ImGui::Button(ICON_FA_STOP " Stop", ImVec2(-1, 0)))
                StopPreview();
        }
        else
        {
            if (ImGui::Button(ICON_FA_PLAY " Play", ImVec2(-1, 0)))
                PlayPreview(hash);
        }

        ImGui::Spacing();

        // Now Playing indicator
        float progress = 0.0f;
        std::string playingName = "None";
        const float ANIMATION_INTERVAL = 0.5f;

        if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
        {
            if (const std::string* previewSoundName{ ST<AssetManager>::Get()->Editor_GetName(lastPreviewAudioHash) })
                playingName = *previewSoundName;

            FMOD::Sound* currentSound = ST<AudioManager>::Get()->GetSound(currentPreviewSound);
            if (currentSound)
            {
                unsigned int currentPos = ST<AudioManager>::Get()->GetChannelPosition(currentPreviewSound);
                unsigned int totalLength = 0;
                currentSound->getLength(&totalLength, FMOD_TIMEUNIT_MS);
                if (totalLength > 0)
                    progress = static_cast<float>(currentPos) / static_cast<float>(totalLength);
            }
        }

        // Animated text
        std::string displayText = playingName;
        if (playingName != "None")
        {
            animationTimer += ImGui::GetIO().DeltaTime;
            if (animationTimer >= ANIMATION_INTERVAL)
            {
                animationTimer = 0.0f;
                tildeCount++;
                if (tildeCount > 3)
                    tildeCount = 0;
            }
            for (int i = 0; i < tildeCount; ++i)
                displayText += "~";
        }

        ImGui::Text("Now Playing:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", displayText.c_str());
        ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // 3D Mode checkbox
        if (ImGui::Checkbox("3D Sound Mode", &use3DMode))
        {
            if (use3DMode)
            {
                ST<AudioManager>::Get()->SetChannel3DAttributes(currentPreviewSound, pos, vel);
                ST<AudioManager>::Get()->SetChannel3D(currentPreviewSound, true);
            }
            else
            {
                ST<AudioManager>::Get()->SetChannel3D(currentPreviewSound, false);
            }
        }

        if (use3DMode)
        {
            ImGui::Text("Position (listener at origin)");
            if (ImGui::DragFloat3("Position", (float*)&pos, 0.1f))
                ST<AudioManager>::Get()->SetChannel3DAttributes(currentPreviewSound, pos, vel);
            if (ImGui::DragFloat3("Velocity", (float*)&vel, 0.1f))
                ST<AudioManager>::Get()->SetChannel3DAttributes(currentPreviewSound, pos, vel);
        }

        // Fade checkbox
        if (ImGui::Checkbox("Fade", &queueFade))
        {
            if (queueFade)
            {
                ST<AudioManager>::Get()->FadeoutAudio(currentPreviewSound, 2.0f);
                queueFade = !queueFade;
            }
        }

        // Volume control
        ImGui::Text("Volume");
        if (ImGui::DragFloat("##Volume", &previewVolume, 0.01f, 0.0f, 1.0f, "%.2f"))
        {
            if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
                ST<AudioManager>::Get()->SetVolume(currentPreviewSound, previewVolume);
        }
#endif
    }

    void SoundTab::PlayPreview(size_t hash)
    {
        // Stop current preview if playing
        if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
        {
            ST<AudioManager>::Get()->StopSound(currentPreviewSound);
            currentPreviewSound = 0;
        }

        // If clicking the same sound that was already playing, just stop (toggle)
        if (hash == lastPreviewAudioHash)
        {
            lastPreviewAudioHash = 0;
            return;
        }

        // Play the new sound
        if (use3DMode)
            currentPreviewSound = ST<AudioManager>::Get()->PlaySound3D(hash, false, Vec3{}, AudioType::END);
        else
            currentPreviewSound = ST<AudioManager>::Get()->PlaySound(hash, false, AudioType::END);

        lastPreviewAudioHash = hash;
    }

    void SoundTab::StopPreview()
    {
        if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
        {
            ST<AudioManager>::Get()->StopSound(currentPreviewSound);
            currentPreviewSound = 0;
            lastPreviewAudioHash = 0;
        }
    }

}
