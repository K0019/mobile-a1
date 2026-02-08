#include "pch.h"  // Engine headers assume the PCH is available
#include <android_native_app_glue.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <cstdint>
#include "core/engine/engine.h"  // Engine<> template and Core::Platform
#include "Engine.h"               // MagicEngine class
#include "renderer/gfx_renderer.h"  // GfxRenderer and RenderFrameData
#include "renderer/features/scene_feature.h"
#include "renderer/features/ui2d_render_feature.h"
#include "math/camera.h"
#include "math/utils_math.h"
#include "VFS/VFS.h"
#include "core/platform/android/ry_android_input_api.h"
#include "Engine/Graphics Interface/GraphicsAPI.h"

#include <stdlib.h> // For using setenv to enable vulkan validation

#ifdef NDEBUG
  #define LOGI(...) ((void)0)
  #define ASSET_LOGI(...) ((void)0)
  #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MagicEngine", __VA_ARGS__)
  #define ASSET_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MagicEngineAssets", __VA_ARGS__)
#else
  #define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MagicEngine", __VA_ARGS__)
  #define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MagicEngine", __VA_ARGS__)
  #define ASSET_LOGI(...) __android_log_print(ANDROID_LOG_INFO, "MagicEngineAssets", __VA_ARGS__)
  #define ASSET_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "MagicEngineAssets", __VA_ARGS__)
#endif


class AndroidApp {
    MagicEngine engine;
    uint64_t sceneFeatureHandle_ = 0;
    uint64_t ui2dFeatureHandle_ = 0;
public:
    void Initialize(Context& context) {
        engine.Init(context, true);  // Start in game mode on Android

        // Create render features for Android (similar to game - no editor features)
        if (context.renderer)
        {
            sceneFeatureHandle_ = context.renderer->CreateFeature<SceneRenderFeature>(false);  // no object picking
            ui2dFeatureHandle_ = context.renderer->CreateFeature<Ui2DRenderFeature>();
            LOGI("Created render features: scene=%lu, ui2d=%lu",
                 (unsigned long)sceneFeatureHandle_, (unsigned long)ui2dFeatureHandle_);

            // Set feature handles on GraphicsMain so UI overlay works
            if (ST<GraphicsMain>::IsInitialized())
            {
                ST<GraphicsMain>::Get()->SetSceneFeatureHandle(sceneFeatureHandle_);
                ST<GraphicsMain>::Get()->SetUI2DFeatureHandle(ui2dFeatureHandle_);
                ST<GraphicsMain>::Get()->InitializeUI2DOverlay();
                LOGI("Initialized UI2D overlay on GraphicsMain");
            }
        }
    }

    void Update(Context& context, RenderFrameData& frame)
    {
        // Setup single-view rendering for Android game
        const int width = static_cast<int>(frame.surface.presentWidth);
        const int height = static_cast<int>(frame.surface.presentHeight);

        if (context.renderer)
        {
            const FeatureMask sceneMask = context.renderer->GetFeatureMask(sceneFeatureHandle_);
            const FeatureMask uiMask = context.renderer->GetFeatureMask(ui2dFeatureHandle_);

            frame.views.resize(1);
            FrameData& gameView = EnsureView(frame, 0);
            gameView.featureMask = sceneMask | uiMask;
            gameView.viewportWidth = static_cast<float>(width);
            gameView.viewportHeight = static_cast<float>(height);
            frame.presentedViewId = gameView.viewId;
        }

        engine.ExecuteFrame(frame);
    }

    void Shutdown(Context& context) {
        LOGI("AndroidApp::Shutdown called");
    }
};

struct EngineContext {
    Engine<AndroidApp>* engine = nullptr;
    bool initialized = false;
};

static void HandleCmd(android_app* app, int32_t cmd) {
    EngineContext* ctx = static_cast<EngineContext*>(app->userData);

    LOGI("HandleCmd: cmd=%d, ctx=%p", cmd, ctx);

    if (!ctx) {
        LOGE("HandleCmd: ctx is NULL!");
        return;
    }

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("APP_CMD_INIT_WINDOW: window=%p, initialized=%d", app->window, ctx->initialized);

            if (app->window && !ctx->initialized) {
                LOGI("Starting initialization sequence");

                Core::Platform::Config config;
                config.appName = "Game App";
                config.enableValidation = false;
#ifdef NDEBUG
                config.logToConsole = false;
                config.logLevel = None;
#else
                config.logToConsole = true;
                config.logLevel = Trace;
#endif
                config.androidApp = app;

                LOGI("Calling Platform::Initialize");
                bool platInit = Core::Platform::Get().Initialize(config);
                LOGI("Platform::Initialize returned: %d", platInit);

                LOGI("Calling Lifecycle::OnResume");
                Core::Platform::Get().GetLifecycle().OnResume();

                // CRITICAL: Create the engine BEFORE setting the window
                // This ensures callbacks are registered
                LOGI("Creating Engine");
                ctx->engine = new Engine<AndroidApp>();

                LOGI("Calling Engine::Initialize");
                ctx->engine->Initialize();
                LOGI("Engine::Initialize returned");

                // NOW set the window - this will trigger onSurfaceCreated callback
                LOGI("Calling Display::SetWindow with %p", app->window);
                Core::Platform::Get().GetDisplay().SetWindow(app->window);
                LOGI("Display::SetWindow returned");

                LOGI("Display IsValid: %d", Core::Platform::Get().GetDisplay().IsValid());
                LOGI("Display dimensions: %dx%d",
                     Core::Platform::Get().GetDisplay().GetWidth(),
                     Core::Platform::Get().GetDisplay().GetHeight());

                ctx->initialized = true;
                LOGI("Initialization complete!");
            } else if (app->window) {
                LOGI("Window already initialized, calling SetWindow");
                Core::Platform::Get().GetDisplay().SetWindow(app->window);
            } else {
                LOGI("APP_CMD_INIT_WINDOW: window is NULL or already initialized");
            }
            break;

        case APP_CMD_TERM_WINDOW:
            LOGI("APP_CMD_TERM_WINDOW");
            Core::Platform::Get().GetDisplay().SetWindow(nullptr);
            break;

        case APP_CMD_WINDOW_RESIZED:
            LOGI("APP_CMD_WINDOW_RESIZED");
            Core::Platform::Get().GetDisplay().UpdateDimensions();
            break;

        case APP_CMD_CONFIG_CHANGED:
            LOGI("APP_CMD_CONFIG_CHANGED");
            Core::Platform::Get().GetDisplay().UpdateDimensions();
            break;

        case APP_CMD_RESUME:
            LOGI("APP_CMD_RESUME");
            Core::Platform::Get().GetLifecycle().OnResume();
            break;

        case APP_CMD_PAUSE:
            LOGI("APP_CMD_PAUSE");
            Core::Platform::Get().GetLifecycle().OnPause();
            break;

        case APP_CMD_STOP:
            LOGI("APP_CMD_STOP");
            Core::Platform::Get().GetLifecycle().OnStop();
            break;

        case APP_CMD_DESTROY:
            LOGI("APP_CMD_DESTROY");
            Core::Platform::Get().GetLifecycle().NotifyDestroy();
            break;

        case APP_CMD_LOW_MEMORY:
            LOGI("APP_CMD_LOW_MEMORY");
            Core::Platform::Get().GetLifecycle().OnLowMemory();
            break;

        default:
            LOGI("Unhandled command: %d", cmd);
            break;
    }
}

static int32_t HandleInput(android_app* app, AInputEvent* event) {
    EngineContext* ctx = static_cast<EngineContext*>(app->userData);
    if (!ctx || !ctx->initialized) return 0;

    int handled_ry = ry_handle_ainput_event(event);                         // <-- feed ry
    int handled_core = Core::Platform::Get().GetInput().HandleInputEvent(event);
    return (handled_ry || handled_core) ? 1 : 0;   // equivalent to (handled_ry | handled_core)

    //return Core::Platform::Get().GetInput().HandleInputEvent(event);

}

void android_main(android_app* app) {
    LOGI("===== android_main starting =====");
    JNIEnv* env = nullptr;
    JavaVM* vm = app->activity->vm;
    jint result = vm->AttachCurrentThread(&env, nullptr);
    if (result != JNI_OK) {
        LOGE("Failed to attach thread to JVM: %d", result);
        return;
    }
    LOGI("Thread attached to JVM");

    AAssetManager* assetManager = app->activity->assetManager;

    ASSET_LOGI("android_main: AssetManager=%p", assetManager);
    if (assetManager)
    {
        // Quick sanity checks to confirm APK asset packaging.
        // 1) Try opening the manifest that AndroidVFSImpl depends on for case-insensitive lookups.
        AAsset* manifest = AAssetManager_open(assetManager, "asset_manifest.txt", AASSET_MODE_STREAMING);
        if (manifest)
        {
            const off_t len = AAsset_getLength(manifest);
            ASSET_LOGI("Sanity: opened asset_manifest.txt (len=%ld)", (long)len);
            AAsset_close(manifest);
        }
        else
        {
            ASSET_LOGE("Sanity: FAILED to open asset_manifest.txt from APK assets");
        }

        // 2) List a few root assets (helps catch wrong sourceSets/assets.srcDirs).
        AAssetDir* rootDir = AAssetManager_openDir(assetManager, "");
        if (rootDir)
        {
            int count = 0;
            const char* filename = nullptr;
            while ((filename = AAssetDir_getNextFileName(rootDir)) != nullptr)
            {
                if (count < 25)
                {
                    ASSET_LOGI("Sanity: root asset[%d]=%s", count, filename);
                }
                ++count;
            }
            ASSET_LOGI("Sanity: root asset count=%d", count);
            AAssetDir_close(rootDir);
        }
        else
        {
            ASSET_LOGE("Sanity: FAILED to open root asset dir");
        }
    }
    else
    {
        ASSET_LOGE("android_main: app->activity->assetManager is NULL");
    }

    VFS::Initialize();
    VFS::MountAndroidDirectory("", assetManager);

    EngineContext ctx;
    app->userData = &ctx;
    app->onAppCmd = HandleCmd;
    app->onInputEvent = HandleInput;

    LOGI("Entering main event loop");
    int frameCount = 0;

    while (true) {
        int events;
        android_poll_source* source;

        // Poll all is deprecated, please check if pollOnce here actually causes problems
        while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }

            if (app->destroyRequested) {
                LOGI("Destroy requested, exiting loop");
                goto cleanup;
            }
        }

        if (ctx.initialized && ctx.engine) {
            //ry_fire_tap_if_any();
            //ry_input_dispatch_frame_events();
          //  ry_fire_tap_if_any();
            ry_pump_touch_events();         // <-- replaces old tick + fire

            Core::Platform::Get().GetInput().Update();

            //ry_tick_android_input();
            //TK Testing this stuff for input
            if (!ctx.engine->ExecuteFrame()) {
                LOGE("ExecuteFrame returned false!");
                break;
            }
        }
    }

    cleanup:
    LOGI("===== Cleanup starting =====");
    if (ctx.engine) {
        ctx.engine->Shutdown();
        delete ctx.engine;
    }

    Core::Platform::Get().Shutdown();
    LOGI("===== android_main exiting =====");
}

extern "C" JNIEXPORT void JNICALL
Java_com_magicengine_kurorekishi_MainActivity_setVulkanLayerPath(
        JNIEnv* env, jobject thiz, jstring layerPathJstr)
{
    // 1. Get the absolute path string from Java
    const char* androidAbsolutePath = env->GetStringUTFChars(layerPathJstr, 0);

    // 2. Set the *absolute* path to VK_LAYER_PATH
    // This variable takes precedence over VK_ADD_LAYER_PATH
    // and correctly points the Vulkan Loader to your copied JSON manifest.
    setenv("VK_LAYER_PATH", androidAbsolutePath, 1);

    __android_log_print(ANDROID_LOG_INFO, "MagicEngine", "VK_LAYER_PATH set to: %s", androidAbsolutePath);

    // 3. Clean up
    env->ReleaseStringUTFChars(layerPathJstr, androidAbsolutePath);
}
