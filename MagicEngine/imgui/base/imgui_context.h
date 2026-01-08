#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <core/engine/engine.h>
#include <resource/resource_types.h>
#include "bindless_registry.h"

struct ImGuiConfig
{
  bool enableKeyboard = true;
  bool enableGamepad = false;
  bool enableDocking = true;
  float displayScale = 1.0f;
};

struct IconFontDesc
{
  std::string filePath;
  std::vector<uint8_t> fontData;
  uint32_t firstCodepoint = 0;
  uint32_t lastCodepoint = 0;
  float sizePixels = 0.0f;
  float glyphMinAdvanceX = 0.0f;
  bool snapToPixel = true;
};

struct ImGuiFontRequest
{
  std::string fontPath;
  std::vector<uint8_t> fontData;
  float fontSize = 13.0f;
  std::vector<uint32_t> extraCodepoints;
  std::vector<IconFontDesc> iconFonts;
};

namespace editor
{
  class TransientRegistry;

  class ImGuiContext
  {
  public:
    ImGuiContext(Context& context, const ImGuiConfig& config = {});

    ~ImGuiContext();

    // Font management - can be called anytime after construction
    void setFont(const ImGuiFontRequest& request);

    void setFont(const char* fontPath, float fontSize);

    void rebuildFontAtlas();

    FontHandle getSharedFontHandle() const;

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

    bool applyFontFromResource(FontHandle handle);

    void updateRenderFeatureParams();
#if defined(__ANDROID__)
    int32_t handleAndroidInput(const AInputEvent* inputEvent);
#endif
    static void applyRendererDefaults(ImFontConfig& config);

    Context& context_;
    ImGuiConfig config_;
    std::unique_ptr<TransientRegistry> m_transientRegistry{};
    uint64_t renderFeatureHandle_;
    TextureHandle fontTextureHandle_{};
    FontHandle sharedFontHandle_{};
    bool ownsFontTexture_ = false;
    bool initialized_;
  };
} // namespace editor
