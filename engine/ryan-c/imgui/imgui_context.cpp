#ifdef IMGUI_ENABLED
#include "imgui_context.h"
#include <imgui.h>
#include "imgui_internal.h"
#if defined(__ANDROID__)
#include <imgui_impl_android.h>
#else
#include <imgui_impl_glfw.h>
#endif
#include <stdexcept>
#include <cassert>
#include <GLFW/glfw3.h>
#include <graphics/renderer.h>

#include "asset_system.h"
#include "Engine.h"
#include "imgui_render_feature.h"
#include "processed_assets.h"

namespace editor {
    ImGuiContext::ImGuiContext(Context& context, GLFWwindow& window, const ImGuiConfig& config)
        : context_(context), window_(window), config_(config),
        m_transientRegistry(std::make_unique<TransientRegistry>()),
        renderFeatureHandle_(0), initialized_(false)
    {
        setupImGuiContext(config);
        setupPlatformBackend();

        // Start with default font
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->AddFontDefault();
        rebuildFontAtlas();

        createRenderFeature();
        context_.renderer->AddTransientResourceObserver(m_transientRegistry.get());
    }

    ImGuiContext::~ImGuiContext() {
        if(initialized_) {
            if(m_transientRegistry && context_.renderer) {
                context_.renderer->RemoveTransientResourceObserver(m_transientRegistry.get());
            }
            if(renderFeatureHandle_ != 0) {
                context_.renderer->DestroyFeature(renderFeatureHandle_);
                renderFeatureHandle_ = 0;
            }

#if defined(__ANDROID__)
            ImGui_ImplAndroid_Shutdown();
#else
            ImGui_ImplGlfw_Shutdown();
#endif
            ImGui::DestroyContext();
            initialized_ = false;
        }
    }

    void ImGuiContext::setFont(const char* fontPath, float fontSize) {
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Clear();

        ImFontConfig cfg = ImFontConfig();
        applyRendererDefaults(cfg);
        cfg.SizePixels = ceilf(fontSize);

        void* data;
        SCOPE_EXIT {if(data) delete data;};
        ImFont* font = nullptr;
        if(fontPath) {
            size_t data_size = 0;
            data = ImFileLoadToMemory(fontPath, "rb", &data_size, 0);
            io.Fonts->AddFontFromMemoryTTF(data, data_size ,cfg.SizePixels, &cfg);

            // save memory by taking ownership of the font data and deleting it the moment this function returns.
            //font = io.Fonts->AddFontFromFileTTF(fontPath, cfg.SizePixels, &cfg);
        }

        // Fallback to default if loading failed
        if(!font) {
            font = io.Fonts->AddFontDefault(&cfg);
        }

        io.FontDefault = font;
        rebuildFontAtlas();
    }

    void ImGuiContext::setupImGuiContext(const ImGuiConfig& config) {
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.BackendPlatformName = "editor_imgui_context";
        io.BackendRendererName = "imgui-render-feature";
        io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

        if(config.enableKeyboard)
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        if(config.enableGamepad)
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
        if(config.enableDocking)
#if defined(__ANDROID__)
#else
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif

        io.FontGlobalScale = config.displayScale;
        io.IniFilename = nullptr;
        initialized_ = true;
    }

    void ImGuiContext::setupPlatformBackend() const {
#if defined(__ANDROID__)
        // Android backend initialization
        // Note: ANativeWindow should be obtained from your Android activity/surface
        ANativeWindow* nativeWindow = window_.GetNativeWindowHandle();
        if(!nativeWindow) {
            throw std::runtime_error("ImGuiContext: Invalid Android native window handle");
        }

        if(!ImGui_ImplAndroid_Init(nativeWindow)) {
            throw std::runtime_error("ImGuiContext: Failed to initialize Android backend");
        }
#else
        // GLFW backend initialization
        GLFWwindow* nativeWindow = &window_;
        if(!nativeWindow) {
            throw std::runtime_error("ImGuiContext: Invalid GLFW window handle");
        }

        if(!ImGui_ImplGlfw_InitForOther(nativeWindow, true)) {
            throw std::runtime_error("ImGuiContext: Failed to initialize GLFW backend");
        }
#endif
    }

    void ImGuiContext::rebuildFontAtlas() {
        assert(initialized_ && "ImGuiContext not initialized");

        ImGuiIO& io = ImGui::GetIO();

        // Ensure we have at least one font
        if(io.Fonts->Fonts.Size == 0) {
            ImFontConfig cfg;
            applyRendererDefaults(cfg);
            io.Fonts->AddFontDefault(&cfg);
            io.FontDefault = io.Fonts->Fonts[0];
        }

        createFontTexture();
        updateRenderFeatureParams();
    }

    void ImGuiContext::createRenderFeature() {
        renderFeatureHandle_ = context_.renderer->CreateFeature<ImGuiRenderFeature>();
        updateRenderFeatureParams();
    }

    void ImGuiContext::createFontTexture() {
        ImGuiIO& io = ImGui::GetIO();

        // Free previous texture if it exists
        if(fontTextureHandle_.isValid()) {
            context_.assetSystem->freeTexture(fontTextureHandle_);
            fontTextureHandle_ = {};
        }

        // Generate font atlas
        unsigned char* pixels;
        int width, height;
        io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        // Create unique identifier for texture caching
        std::string fontHash = std::to_string(std::hash<std::string>{}(
            std::string(reinterpret_cast<const char*>(pixels), width * height * 4)));

        AssetLoading::ProcessedTexture texture;
        texture.name = "ImGuiFontAtlas";
        texture.source = EmbeddedMemorySource{
            .identifier = "FontAtlas_" + fontHash,
            .scenePath = "ImGuiContext"
        };
        texture.textureDesc = {
            .type = vk::TextureType::Tex2D,
            .format = vk::Format::RGBA_UN8,
            .dimensions = {
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height)
            },
            .usage = vk::TextureUsageBits_Sampled
        };
        texture.data = std::vector(pixels, pixels + (width * height * 4));

        fontTextureHandle_ = context_.assetSystem->createTexture(texture);
        io.Fonts->TexID = context_.assetSystem->getTextureBindlessIndex(fontTextureHandle_);
    }

    void ImGuiContext::beginFrame() const {
        assert(initialized_ && "ImGuiContext not initialized");

        ImGuiIO& io = ImGui::GetIO();

#if defined(__ANDROID__)
        // Android doesn't need manual display size setup in the same way
        // The android backend handles display metrics internally
        ImGui_ImplAndroid_NewFrame();
#else
        // GLFW backend requires manual display size setup
        int width, height;
        glfwGetFramebufferSize(&window_, &width, &height);
        io.DisplaySize = ImVec2(width / config_.displayScale, height / config_.displayScale);
        io.DisplayFramebufferScale = ImVec2(config_.displayScale, config_.displayScale);

        ImGui_ImplGlfw_NewFrame();
#endif
        ImGui::NewFrame();
    }

    void ImGuiContext::endFrame() {
        assert(initialized_ && "ImGuiContext not initialized");

        ImGui::EndFrame();
        ImGui::Render();

        if(renderFeatureHandle_ != 0) {
            ImGuiRenderParams* params = static_cast<ImGuiRenderParams*>(
                context_.renderer->GetFeatureParameterBlockPtr(renderFeatureHandle_));
            if(params) {
                params->hasNewFrame = true;
            }
        }
    }

#if defined(__ANDROID__)
    int32_t ImGuiContext::handleAndroidInput(const AInputEvent* inputEvent) {
        assert(initialized_ && "ImGuiContext not initialized");
        return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
    }
#endif

    void ImGuiContext::updateRenderFeatureParams() {
        if(renderFeatureHandle_ == 0)
            return;

        ImGuiRenderParams* params = static_cast<ImGuiRenderParams*>(
            context_.renderer->GetFeatureParameterBlockPtr(renderFeatureHandle_));

        if(params) {
            params->enabled = true;
        }
    }

    void ImGuiContext::applyRendererDefaults(ImFontConfig& config) {
        config.FontDataOwnedByAtlas = false;  // Renderer owns font data
        config.RasterizerMultiply = 1.5f;     // Good quality for most fonts
        config.PixelSnapH = true;             // Better alignment
        config.OversampleH = 4;               // High quality rendering
        config.OversampleV = 4;               // High quality rendering
    }

    void ImGuiContext::setDisplayScale(float scale) {
        config_.displayScale = scale;
    }

    float ImGuiContext::getDisplayScale() const {
        return config_.displayScale;
    }

    bool ImGuiContext::wantCaptureMouse() const {
        assert(initialized_ && "ImGuiContext not initialized");
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGuiContext::wantCaptureKeyboard() const {
        assert(initialized_ && "ImGuiContext not initialized");
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    bool ImGuiContext::wantTextInput() const {
        assert(initialized_ && "ImGuiContext not initialized");
        return ImGui::GetIO().WantTextInput;
    }

    TransientRegistry& ImGuiContext::GetTransientRegistry() {
        return *m_transientRegistry;
    }
}
#endif