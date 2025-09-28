#include "imgui_context.h"

#ifdef IMGUI_ENABLED

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <stdexcept>
#include <cassert>
#include <graphics/renderer.h>
#include "imgui_render_feature.h"
#include "assets/core/asset_system.h"
#include "assets/io/texture_loader.h"

namespace editor {
    ImGuiContext::ImGuiContext(Context& context, GLFWwindow& window, const ImGuiConfig& config)
        : context_(context), window_(window), config_(config),
        m_transientRegistry(std::make_unique<TransientRegistry>()),
        renderFeatureHandle_(0), initialized_(false)
    {
        setupImGuiContext(config);
        setupGLFWBackend();

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

            ImGui_ImplGlfw_Shutdown();
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

        ImFont* font = nullptr;
        if(fontPath) {
            font = io.Fonts->AddFontFromFileTTF(fontPath, cfg.SizePixels, &cfg);
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
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

        io.FontGlobalScale = config.displayScale;
        io.IniFilename = nullptr;
        initialized_ = true;
    }

    void ImGuiContext::setupGLFWBackend() const {
        GLFWwindow* nativeWindow = &window_;
        if(!nativeWindow) {
            throw std::runtime_error("ImGuiContext: Invalid GLFW window handle");
        }

        if(!ImGui_ImplGlfw_InitForOther(nativeWindow, true)) {
            throw std::runtime_error("ImGuiContext: Failed to initialize GLFW backend");
        }
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
        int width, height;
        glfwGetFramebufferSize(&window_, &width, &height);
        io.DisplaySize = ImVec2(width / config_.displayScale, height / config_.displayScale);
        io.DisplayFramebufferScale = ImVec2(config_.displayScale, config_.displayScale);

        ImGui_ImplGlfw_NewFrame();
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
