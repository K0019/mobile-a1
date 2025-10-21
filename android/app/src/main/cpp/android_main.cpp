#include <android_native_app_glue.h>
#include <android/log.h>
#include "engine.h"
#include "math/camera.h"
#include "math/utils_math.h"
#include <graphics/features/grid_feature.h>
#include <base/imgui_context.h>
#include "VFS/VFS.h"
#include "MagicEngine/Engine/Engine.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ryEngine", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ryEngine", __VA_ARGS__)

class AndroidApp {
    MagicEngine engine;
public:
    void Initialize(Context& context) {
        //engine.Init(context);
    }

    void Update(Context& context, FrameData& frame) {
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
                config.appName = "ryEngine App";
                config.enableValidation = false;
                config.logToConsole = true;
                config.logLevel = Trace;
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

    return Core::Platform::Get().GetInput().HandleInputEvent(event);
}

void android_main(android_app* app) {
    LOGI("===== android_main starting =====");

    EngineContext ctx;
    app->userData = &ctx;
    app->onAppCmd = HandleCmd;
    app->onInputEvent = HandleInput;

    AAssetManager* assetManager = app->activity->assetManager;
    VFS::Initialize();
    VFS::MountAndroidDirectory("assets", assetManager);


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
            Core::Platform::Get().GetInput().Update();

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