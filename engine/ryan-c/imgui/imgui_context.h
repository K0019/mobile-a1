#pragma once

#ifdef IMGUI_ENABLED

#include <memory>

#include "asset_types.h"
#include "ImguiHeader.h"
#include "imgui_render_feature.h"
#include "bindless_registry.h"
#include "context.h"

struct ImGuiConfig {
    bool enableKeyboard = true;
    bool enableGamepad = false;
    bool enableDocking = true;
    float displayScale = 1.0f;
};

namespace editor {
    class TransientRegistry;

    class ImGuiContext {
    public:
        ImGuiContext(Context& context, GLFWwindow& window, const ImGuiConfig& config = {});
        ~ImGuiContext();

        // Font management - can be called anytime after construction
        void setFont(const char* fontPath, float fontSize);
        void rebuildFontAtlas();

        // Frame management
        void beginFrame() const;
        void endFrame();

        // Display scale management
        void setDisplayScale(float scale);
        float getDisplayScale() const;

        // Input capture queries
        bool wantCaptureMouse() const;
        bool wantCaptureKeyboard() const;
        bool wantTextInput() const;

        TransientRegistry& GetTransientRegistry();
        private:
        void setupImGuiContext(const ImGuiConfig& config);
        void setupPlatformBackend() const;
        void createRenderFeature();
        void createFontTexture();
        void updateRenderFeatureParams();

#if defined(__ANDROID__)
        int32_t handleAndroidInput(const AInputEvent* inputEvent);
#endif

        static void applyRendererDefaults(ImFontConfig& config);

        Context& context_;
        GLFWwindow& window_;
        ImGuiConfig config_;

        std::unique_ptr<TransientRegistry> m_transientRegistry;
        uint64_t renderFeatureHandle_;
        TextureHandle fontTextureHandle_;
        bool initialized_;
    };
}
#endif