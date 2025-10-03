#include "SoundTab.h"
#include "ResourceManager.h"
#include "GUICollection.h"

const char* SoundTab::GetName() const
{
    return "Sounds";
}

const char* SoundTab::GetIdentifier() const
{
    return ICON_FA_MICROPHONE" Sounds";
}

void SoundTab::Render()
{
    gui::Child child{ "SoundTable", gui::Vec2{ 0.0f, -FLT_MIN }, gui::FLAG_CHILD::BORDERS };

#ifdef IMGUI_ENABLED
    auto soundResources{ ST<ResourceManager>::Get()->Editor_GetAudio().Editor_GetAllResources() };

    ImGui::Columns(2, nullptr, true);

    // Left column for single sounds
    ImGui::Text("Sounds");

    for (const auto& [hash, resource] : soundResources)
    {
        const std::string& name{ ST<ResourceManager>::Get()->Editor_GetName(hash) };

        // Create Button
        if (ImGui::Selectable(name.c_str()))
        {
            if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
            {
                ST<AudioManager>::Get()->StopSound(currentPreviewSound);
                currentPreviewSound = 0;
            }

            if (hash == lastPreviewAudioHash)
            {
                lastPreviewAudioHash = 0;
            }
            else
            {
                if (use3DMode)
                    currentPreviewSound = ST<AudioManager>::Get()->PlaySound3D(hash, false, Vec3{}, AudioType::END);
                else
                    currentPreviewSound = ST<AudioManager>::Get()->PlaySound(hash, false, AudioType::END);

                lastPreviewAudioHash = hash;
            }
        }

        // Drag-drop source
        gui::PayloadSource{ "SOUND_HASH", hash.get(), name.c_str()};

        // Context menu
        RenderSoundContextMenu(hash, name);
    }

    ImGui::NextColumn();

    // Right column for audio controls
    static float previewVolume = 1.f;
    static Vec3 pos = Vec3{ 0.f, 0.f, 0.f };
    static Vec3 vel = Vec3{ 0.f, 0.f, 0.f };

    ImGui::Text("Audio Controls");
    ImGui::Separator();

    // Now Playing section
    float progress = 0.0f;
    std::string playingName = "None";
    static float animationTimer = 0.0f;
    static int tildeCount = 0;
    const float ANIMATION_INTERVAL = 0.5f; // Add a tilde every half-second

    if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
    {
        playingName = ST<ResourceManager>::Get()->Editor_GetName(lastPreviewAudioHash);
        FMOD::Sound* currentSound = ST<AudioManager>::Get()->GetSound(currentPreviewSound);

        if (currentSound)
        {
            // Get current position and total length in milliseconds
            unsigned int currentPos = ST<AudioManager>::Get()->GetChannelPosition(currentPreviewSound);

            unsigned int totalLength = 0;
            currentSound->getLength(&totalLength, FMOD_TIMEUNIT_MS);

            // Calculate progress as a fraction (0.0 to 1.0)
            if (totalLength > 0)
            {
                progress = static_cast<float>(currentPos) / static_cast<float>(totalLength);
            }
        }
    }

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

    // Track play %
    // The ImVec2(-1, 0) makes the progress bar fill the available width.
    ImGui::ProgressBar(progress, ImVec2(-1.0f, 0.0f));

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

    ImGui::Text("Sound Position (Assumes listener is at origin)");
    if (ImGui::DragFloat3("Position", (float*)&pos, 0.1f))
    {
        ST<AudioManager>::Get()->SetChannel3DAttributes(currentPreviewSound, pos, vel);
    }
    if (ImGui::DragFloat3("Velocity", (float*)&vel, 0.1f))
    {
        ST<AudioManager>::Get()->SetChannel3DAttributes(currentPreviewSound, pos, vel);
    }

    // Volume control
    ImGui::Text("Volume");
    if (ImGui::DragFloat("##Volume", &previewVolume, 0.01f, 0.0f, 1.0f, "%.2f"))
    {
        if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
        {
            ST<AudioManager>::Get()->SetVolume(currentPreviewSound, previewVolume);
        }
    }

    // Stop button
    ImGui::Separator();
    if (ImGui::Button("Stop Preview", ImVec2(-1, 0)))
    {
        if (ST<AudioManager>::Get()->IsPlaying(currentPreviewSound))
        {
            ST<AudioManager>::Get()->StopSound(currentPreviewSound);
            currentPreviewSound = 0;
            lastPreviewAudioHash = 0;
        }
    }
#endif
}

void SoundTab::RenderSoundContextMenu(size_t hash, const std::string& name)
{
    if (gui::ItemContextMenu contextMenu{ ("Delete##" + name).c_str() })
        if (gui::MenuItem("Delete"))
            // Note: The deletion of the resource calls AudioManager::FreeSound(), which stops all sounds, even sounds that are not this sound being deleted
            ST<ResourceManager>::Get()->INTERNAL_GetAudio().DeleteResource(hash);
}
