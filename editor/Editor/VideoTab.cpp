#include "Editor/VideoTab.h"
#include "Assets/AssetManager.h"

namespace editor {

    const AssetTabConfig VideoTab::config = {
        .name = "Videos",
        .identifier = ICON_FA_VIDEO " Videos",
        .icon = ICON_FA_VIDEO,
        .payloadType = "VIDEO_HASH",
        .iconColor = {0.7f, 0.3f, 0.9f, 1.0f},  // Purple
        .thumbnailType = ThumbnailCache::AssetType::Texture,
        .hasThumbnails = false
    };

    const AssetTabConfig& VideoTab::GetConfig() const
    {
        return config;
    }

    void VideoTab::RenderDetailPanelContent(size_t hash, [[maybe_unused]] const std::string& name)
    {
#ifdef IMGUI_ENABLED
        ResourceVideo* video = ST<AssetManager>::Get()->INTERNAL_GetContainer<ResourceVideo>().INTERNAL_GetResource(hash, false);
        if (!video)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Resource not found");
            return;
        }

        // Ensure metadata is loaded (reads from file header if needed)
        if (!video->EnsureMetadataLoaded())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), ICON_FA_CIRCLE_EXCLAMATION " Failed to read video");
            ImGui::TextDisabled("Could not read video file header.");
            return;
        }

        const VideoData& data = video->data;

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Resolution:");
        ImGui::SameLine(100);
        ImGui::Text("%u x %u", data.width, data.height);

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Duration:");
        ImGui::SameLine(100);
        int minutes = static_cast<int>(data.duration) / 60;
        int seconds = static_cast<int>(data.duration) % 60;
        ImGui::Text("%d:%02d", minutes, seconds);

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Frame Rate:");
        ImGui::SameLine(100);
        ImGui::Text("%.1f fps", data.frameRate);

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Codec:");
        ImGui::SameLine(100);
        ImGui::Text("%s", data.codec.empty() ? "Unknown" : data.codec.c_str());

        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Audio:");
        ImGui::SameLine(100);
        ImGui::Text("%s", data.hasAudio ? "Yes" : "No");
#endif
    }

}
