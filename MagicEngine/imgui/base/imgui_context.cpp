#include "imgui_context.h"
#include <imgui.h>
#include "imgui_internal.h"
#include <stdexcept>
#include <cassert>
#include <filesystem>
#include <vector>
#include <string>
#include <utility>
#include <cmath>
#include <algorithm>
#include <cstring>
#include "imgui_render_feature.h"
#if defined(__ANDROID__)
#include "imgui_impl_android.h"
#else
#include "imgui_impl_glfw.h"
#endif
namespace
{
  bool loadFontBytes(const std::string& path, std::vector<uint8_t>& out)
  {
    if (path.empty()) return false;
    size_t dataSize = 0;
    void* rawData = ImFileLoadToMemory(path.c_str(), "rb", &dataSize, 0);
    if (!rawData || dataSize == 0)
    {
      if (rawData)
      {
        IM_FREE(rawData);
      }
      return false;
    }
    out.assign(static_cast<uint8_t*>(rawData), static_cast<uint8_t*>(rawData) + dataSize);
    IM_FREE(rawData);
    return true;
  }
} // namespace
namespace editor
{
  ImGuiContext::ImGuiContext(Context& context, const ImGuiConfig& config)
    : context_(context), config_(config), m_transientRegistry(std::make_unique<TransientRegistry>()),
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

  ImGuiContext::~ImGuiContext()
  {
    if (initialized_)
    {
      if (m_transientRegistry && context_.renderer)
      {
        context_.renderer->RemoveTransientResourceObserver(m_transientRegistry.get());
      }
      if (renderFeatureHandle_ != 0)
      {
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

  void ImGuiContext::setFont(const ImGuiFontRequest& request)
  {
    ImGuiIO& io = ImGui::GetIO();
    sharedFontHandle_ = {};
    const float resolvedFontSize = request.fontSize > 0.0f ? request.fontSize : 13.0f;
    std::vector<uint8_t> fontBytes = request.fontData;
    if (fontBytes.empty())
    {
      loadFontBytes(request.fontPath, fontBytes);
    }
    if (!fontBytes.empty())
    {
      Resource::ProcessedFont processed;
      if (!request.fontPath.empty())
      {
        processed.name = std::filesystem::path(request.fontPath).filename().string();
      }
      if (processed.name.empty())
      {
        processed.name = "ImGuiFont";
      }
      processed.fontFileData = std::move(fontBytes);
      processed.buildSettings.pixelHeight = std::ceil(resolvedFontSize);
      processed.buildSettings.firstCodepoint = 32;
      processed.buildSettings.lastCodepoint = 255;
      processed.buildSettings.fallbackCodepoint = '?';
      processed.buildSettings.snapToPixel = true;
      processed.buildSettings.extraCodepoints = request.extraCodepoints;
      if (std::find(processed.buildSettings.extraCodepoints.begin(), processed.buildSettings.extraCodepoints.end(),
                    0x2026u) == processed.buildSettings.extraCodepoints.end())
      {
        processed.buildSettings.extraCodepoints.push_back(0x2026u);
      }
      processed.sourceFile = request.fontPath;
      processed.mergeSources.reserve(request.iconFonts.size());
      for (const IconFontDesc& icon : request.iconFonts)
      {
        if (icon.firstCodepoint == 0 || icon.lastCodepoint == 0 || icon.firstCodepoint > icon.lastCodepoint)
        {
          continue;
        }
        std::vector<uint8_t> iconBytes = icon.fontData;
        if (iconBytes.empty())
        {
          loadFontBytes(icon.filePath, iconBytes);
        }
        if (iconBytes.empty())
        {
          continue;
        }
        Resource::FontMergeSource merge;
        merge.fontFileData = std::move(iconBytes);
        merge.buildSettings.pixelHeight = std::ceil(icon.sizePixels > 0.0f ? icon.sizePixels : resolvedFontSize);
        merge.buildSettings.firstCodepoint = icon.firstCodepoint;
        merge.buildSettings.lastCodepoint = icon.lastCodepoint;
        merge.buildSettings.fallbackCodepoint = icon.firstCodepoint;
        merge.buildSettings.snapToPixel = icon.snapToPixel;
        merge.buildSettings.enableKerning = false;
        merge.buildSettings.glyphMinAdvanceX = icon.glyphMinAdvanceX;
        merge.sourceFile = icon.filePath;
        if (!icon.filePath.empty())
        {
          merge.name = std::filesystem::path(icon.filePath).filename().string();
        }
        else
        {
          merge.name = processed.name + "_Icon_" + std::to_string(processed.mergeSources.size());
        }
        processed.mergeSources.push_back(std::move(merge));
      }
      sharedFontHandle_ = context_.resourceMngr->createFont(processed);
      if (sharedFontHandle_.isValid() && applyFontFromResource(sharedFontHandle_))
      {
        updateRenderFeatureParams();
        return;
      }
    }
    io.Fonts->Clear();
    ImFontConfig cfg = ImFontConfig();
    applyRendererDefaults(cfg);
    cfg.SizePixels = ceilf(resolvedFontSize);
    ImFont* font = io.Fonts->AddFontDefault(&cfg);
    io.FontDefault = font;
    rebuildFontAtlas();
  }

  void ImGuiContext::setFont(const char* fontPath, float fontSize)
  {
    ImGuiFontRequest request;
    request.fontSize = fontSize;
    if (fontPath && fontPath[0] != '\0')
    {
      request.fontPath = fontPath;
    }
    setFont(request);
  }

  FontHandle ImGuiContext::getSharedFontHandle() const
  {
    return sharedFontHandle_;
  }

  void ImGuiContext::setupImGuiContext(const ImGuiConfig& config)
  {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.BackendPlatformName = "editor_imgui_context";
    io.BackendRendererName = "imgui-render-feature";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    if (config.enableKeyboard) io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (config.enableGamepad) io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    if (config.enableDocking)
#if defined(__ANDROID__)
#else
      io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
#endif
    io.FontGlobalScale = config.displayScale;
    io.IniFilename = nullptr;
    initialized_ = true;
  }

  void ImGuiContext::setupPlatformBackend() const
  {
#if defined(__ANDROID__)
    // Android backend initialization
    // Note: ANativeWindow should be obtained from your Android activity/surface
    ANativeWindow* nativeWindow = Core::Display().GetBackendHandle(); if (!nativeWindow)
    {
      throw std::runtime_error("ImGuiContext: Invalid Android native window handle");
    } if (!ImGui_ImplAndroid_Init(nativeWindow))
    {
      throw std::runtime_error("ImGuiContext: Failed to initialize Android backend");
    }
#else
    // GLFW backend initialization
    GLFWwindow* nativeWindow = Core::Display().GetBackendHandle();
    if (!nativeWindow)
    {
      throw std::runtime_error("ImGuiContext: Invalid GLFW window handle");
    }
    if (!ImGui_ImplGlfw_InitForOther(nativeWindow, true))
    {
      throw std::runtime_error("ImGuiContext: Failed to initialize GLFW backend");
    }
#endif
  }

  void ImGuiContext::rebuildFontAtlas()
  {
    assert(initialized_ && "ImGuiContext not initialized");
    ImGuiIO& io = ImGui::GetIO();
    if (sharedFontHandle_.isValid() && applyFontFromResource(sharedFontHandle_))
    {
      updateRenderFeatureParams();
      return;
    }
    // Ensure we have at least one font
    if (io.Fonts->Fonts.Size == 0)
    {
      ImFontConfig cfg;
      applyRendererDefaults(cfg);
      io.Fonts->AddFontDefault(&cfg);
      io.FontDefault = io.Fonts->Fonts[0];
    }
    createFontTexture();
    updateRenderFeatureParams();
  }

  void ImGuiContext::createRenderFeature()
  {
    renderFeatureHandle_ = context_.renderer->CreateFeature<ImGuiRenderFeature>();
    updateRenderFeatureParams();
  }

  bool ImGuiContext::applyFontFromResource(FontHandle handle)
  {
    if (!handle.isValid()) return false;
    const auto* hot = context_.resourceMngr->getFont(handle);
    if (!hot) return false;
    const Resource::FontCPUData& cpuData = hot->cpuData;
    ImGuiIO& io = ImGui::GetIO();
    ImFontAtlas* atlas = io.Fonts;
    atlas->Clear();
    atlas->Sources.clear();
    ImFontConfig resourceConfig{};
    applyRendererDefaults(resourceConfig);
    const int oversample = (std::max)(1, cpuData.buildSettings.oversample);
    resourceConfig.OversampleH = oversample;
    resourceConfig.OversampleV = oversample;
    resourceConfig.PixelSnapH = cpuData.buildSettings.snapToPixel;
    resourceConfig.RasterizerMultiply = cpuData.buildSettings.rasterizerMultiply;
    resourceConfig.SizePixels = cpuData.buildSettings.pixelHeight;
    if (!cpuData.name.empty())
    {
      ImStrncpy(resourceConfig.Name, cpuData.name.c_str(), IM_ARRAYSIZE(resourceConfig.Name));
    }
    else
    {
      ImFormatString(resourceConfig.Name, IM_ARRAYSIZE(resourceConfig.Name), "ResourceFont");
    }
    atlas->Sources.push_back(resourceConfig);
    ImFontConfig* sourceEntry = atlas->Sources.empty() ? nullptr : &atlas->Sources.back();
    if (!sourceEntry) return false;
    atlas->TexID = static_cast<ImTextureID>(hot->bindlessIndex);
    atlas->TexWidth = cpuData.atlasWidth;
    atlas->TexHeight = cpuData.atlasHeight;
    if (atlas->TexWidth > 0 && atlas->TexHeight > 0)
    {
      atlas->TexUvScale = ImVec2(1.0f / static_cast<float>(atlas->TexWidth),
                                 1.0f / static_cast<float>(atlas->TexHeight));
    }
    else
    {
      atlas->TexUvScale = ImVec2(0.0f, 0.0f);
    }
    atlas->TexUvWhitePixel = ImVec2(cpuData.whitePixelUV.x, cpuData.whitePixelUV.y);
    atlas->TexPixelsUseColors = true;
    unsigned char* atlasPixelCopy = nullptr;
    if (!cpuData.atlasPixelsRGBA.empty())
    {
      const size_t byteCount = cpuData.atlasPixelsRGBA.size();
      atlasPixelCopy = static_cast<unsigned char*>(ImGui::MemAlloc(byteCount));
      if (!atlasPixelCopy)
      {
        IM_ASSERT(false && "Failed to allocate font atlas pixels");
        return false;
      }
      std::memcpy(atlasPixelCopy, cpuData.atlasPixelsRGBA.data(), byteCount);
    }
    atlas->TexPixelsRGBA32 = reinterpret_cast<unsigned int*>(atlasPixelCopy);
    atlas->TexPixelsAlpha8 = nullptr;
    atlas->TexReady = true;
    ImFont* font = IM_NEW(ImFont);
    sourceEntry->DstFont = font;
    font->ContainerAtlas = atlas;
    font->Sources = sourceEntry;
    font->SourcesCount = 1;
    font->FontSize = cpuData.buildSettings.pixelHeight;
    const float ascent = std::ceil(cpuData.ascent);
    const float descent = std::floor(cpuData.descent);
    font->Ascent = ascent;
    font->Descent = descent;
    font->FallbackChar = static_cast<ImWchar>(cpuData.buildSettings.fallbackCodepoint);
    font->Scale = 1.0f;
    font->Glyphs.clear();
    font->IndexAdvanceX.clear();
    font->IndexLookup.clear();
    font->DirtyLookupTables = true;
    const float fontOffsetX = sourceEntry->GlyphOffset.x;
    const float fontOffsetY = sourceEntry->GlyphOffset.y + IM_ROUND(font->Ascent);
    const float invOversampleX = oversample > 0 ? 1.0f / static_cast<float>(oversample) : 1.0f;
    const float invOversampleY = invOversampleX;
    for (const auto& src : cpuData.glyphs)
    {
      const float x0 = src.bearingPx.x + fontOffsetX;
      const float y0 = src.bearingPx.y + fontOffsetY;
      const float x1 = x0 + (src.sizePx.x * invOversampleX);
      const float y1 = y0 + (src.sizePx.y * invOversampleY);
      font->AddGlyph(sourceEntry, static_cast<ImWchar>(src.codepoint), x0, y0, x1, y1, src.uvMin.x, src.uvMin.y,
                     src.uvMax.x, src.uvMax.y, src.advancePx);
    }
    font->BuildLookupTable();
    if (ImFontGlyph* fallback = font->FindGlyph(font->FallbackChar))
    {
      font->FallbackGlyph = fallback;
      font->FallbackAdvanceX = fallback->AdvanceX;
    }
    atlas->Fonts.clear();
    atlas->Fonts.push_back(font);
    for (auto& lineUv : atlas->TexUvLines)
    {
      lineUv = ImVec4(atlas->TexUvWhitePixel.x, atlas->TexUvWhitePixel.y, atlas->TexUvWhitePixel.x,
                      atlas->TexUvWhitePixel.y);
    }
    io.FontDefault = font;
    io.Fonts->TexID = static_cast<ImTextureID>(hot->bindlessIndex);
    fontTextureHandle_ = {};
    ownsFontTexture_ = false;
    return true;
  }

  void ImGuiContext::createFontTexture()
  {
    ImGuiIO& io = ImGui::GetIO();
    // Free previous texture if it exists
    if (ownsFontTexture_ && fontTextureHandle_.isValid())
    {
      context_.resourceMngr->freeTexture(fontTextureHandle_);
      fontTextureHandle_ = {};
    }
    if (sharedFontHandle_.isValid())
    {
      ownsFontTexture_ = false;
      fontTextureHandle_ = {};
      io.Fonts->TexID = context_.resourceMngr->getFontTextureBindlessIndex(sharedFontHandle_);
      return;
    }
    // Generate font atlas
    unsigned char* pixels;
    int width, height;
    io.Fonts->Flags |= ImFontAtlasFlags_NoPowerOfTwoHeight;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    // Create unique identifier for texture caching
    std::string fontHash = std::to_string(std::hash<std::string>{}(
      std::string(reinterpret_cast<const char*>(pixels), width * height * 4)));
    Resource::ProcessedTexture texture;
    texture.name = "ImGuiFontAtlas";
    texture.source = EmbeddedMemorySource{.identifier = "FontAtlas_" + fontHash, .scenePath = "ImGuiContext"};
    texture.textureDesc = {
      .type = vk::TextureType::Tex2D, .format = vk::Format::RGBA_UN8,
      .dimensions = {.width = static_cast<uint32_t>(width), .height = static_cast<uint32_t>(height)},
      .usage = vk::TextureUsageBits_Sampled
    };
    texture.data = std::vector(pixels, pixels + (width * height * 4));
    fontTextureHandle_ = context_.resourceMngr->createTexture(texture);
    ownsFontTexture_ = fontTextureHandle_.isValid();
    io.Fonts->TexID = context_.resourceMngr->getTextureBindlessIndex(fontTextureHandle_);
  }

  void ImGuiContext::beginFrame() const
  {
    assert(initialized_ && "ImGuiContext not initialized");
    ImGuiIO& io = ImGui::GetIO();
#if defined(__ANDROID__)
    // Android doesn't need manual display size setup in the same way
    // The android backend handles display metrics internally
    ImGui_ImplAndroid_NewFrame();
#else
    // GLFW backend requires manual display size setup
    int width = Core::Display().GetWidth();
    int height = Core::Display().GetHeight();
    io.DisplaySize = ImVec2(width / config_.displayScale, height / config_.displayScale);
    io.DisplayFramebufferScale = ImVec2(config_.displayScale, config_.displayScale);
    ImGui_ImplGlfw_NewFrame();
#endif
    ImGui::NewFrame();
  }

  void ImGuiContext::endFrame()
  {
    assert(initialized_ && "ImGuiContext not initialized");
    ImGui::EndFrame();
    ImGui::Render();
    if (renderFeatureHandle_ != 0)
    {
      ImGuiRenderParams* params = static_cast<ImGuiRenderParams*>(context_.renderer->GetFeatureParameterBlockPtr(
        renderFeatureHandle_));
      if (params)
      {
        params->hasNewFrame = true;
      }
    }
  }
#if defined(__ANDROID__)
  int32_t ImGuiContext::handleAndroidInput(const AInputEvent* inputEvent)
  {
    assert(initialized_ && "ImGuiContext not initialized");
    return ImGui_ImplAndroid_HandleInputEvent(inputEvent);
  }
#endif
  void ImGuiContext::updateRenderFeatureParams()
  {
    if (renderFeatureHandle_ == 0) return;
    ImGuiRenderParams* params = static_cast<ImGuiRenderParams*>(context_.renderer->GetFeatureParameterBlockPtr(
      renderFeatureHandle_));
    if (params)
    {
      params->enabled = true;
    }
  }

  void ImGuiContext::applyRendererDefaults(ImFontConfig& config)
  {
    config.FontDataOwnedByAtlas = false; // Renderer owns font data
    config.RasterizerMultiply = 1.5f; // Good quality for most fonts
    config.PixelSnapH = true; // Better alignment
    config.OversampleH = 4; // High quality rendering
    config.OversampleV = 4; // High quality rendering
  }

  void ImGuiContext::setDisplayScale(float scale)
  {
    config_.displayScale = scale;
  }

  float ImGuiContext::getDisplayScale() const
  {
    return config_.displayScale;
  }

  bool ImGuiContext::wantCaptureMouse() const
  {
    assert(initialized_ && "ImGuiContext not initialized");
    return ImGui::GetIO().WantCaptureMouse;
  }

  bool ImGuiContext::wantCaptureKeyboard() const
  {
    assert(initialized_ && "ImGuiContext not initialized");
    return ImGui::GetIO().WantCaptureKeyboard;
  }

  bool ImGuiContext::wantTextInput() const
  {
    assert(initialized_ && "ImGuiContext not initialized");
    return ImGui::GetIO().WantTextInput;
  }

  TransientRegistry& ImGuiContext::GetTransientRegistry()
  {
    return *m_transientRegistry;
  }
} // namespace editor
