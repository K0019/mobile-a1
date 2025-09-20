#pragma once
#include "renderer.h"
#include "asset_system.h"
#include "imgui_context.h"

class GraphicsAssets
{
public:
    bool Init(Context* context);

public:
    AssetLoading::AssetSystem* INTERNAL_GetAssetSystem();

private:
    UPtr<AssetLoading::AssetSystem> assetSystem;

};

class GraphicsMain
{
public:
    ~GraphicsMain();

    bool Init();

    void BeginFrame();
#ifdef IMGUI_ENABLED
    void BeginImGuiFrame();
    void EndImGuiFrame();
#endif
    void EndFrame();

    void SetPendingShutdown();
    bool GetIsPendingShutdown() const;

    bool GetIsWindowMinimized() const;

    bool SetWindowIcon(const std::string& filepath);
    void SetWindowResolution(int width, int height);
    void SetFullscreen(bool isFullscreen);
    void BringWindowToFront();

    void SetCallback_DragDrop(GLFWdropfun func);

private:
    void InitGLFW();
    void SetupGLFWSettings();
    void SetupGLFWCallbacks();
#ifdef IMGUI_ENABLED
    void InitImGui(const std::string& fontfile);
    void SetImGuiStyle();
#endif

    static void GLFW_Callback_FramebufferResize(GLFWwindow* window, int width, int height);
    void Callback_FramebufferResize(int width, int height);
    static void GLFW_Callback_WindowFocusChanged(GLFWwindow* window, int isFocused);
    static void GLFW_Callback_IconifyStateChanged(GLFWwindow* window, int isIconified);
    static void GLFW_Callback_CursorEnteredWindow(GLFWwindow* window, int isEntered);

public:
    // For compatibility with whatever old graphics interfaces are still here
    // Remove if possible once everything settles
    VkExtent2D GetWindowExtent() const;
    VkExtent2D GetViewportExtent() const;
    VkExtent2D GetWorldExtent() const;
    editor::ImGuiContext& GetImGuiContext();
    void LoadSampleScene();
    void RenderSampleScene();

private:
    friend ST<GraphicsMain>;
    GraphicsMain();

private:
    GLFWwindow* window;
    GLFWmonitor* monitor;

    UPtr<Renderer> renderer;
#ifdef IMGUI_ENABLED
    UPtr<editor::ImGuiContext> imguiContext;
#endif

    Context context;
    // remove these at some point, kind of atrocious.
    VkExtent2D windowExtent;
    VkExtent2D viewportExtent;
    VkExtent2D worldExtent;

};

