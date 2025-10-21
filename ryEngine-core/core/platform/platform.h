// platform/platform.h
#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS 1
#define PLATFORM_DESKTOP 1
#elif defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE || TARGET_OS_SIMULATOR
#define PLATFORM_IOS 1
#define PLATFORM_MOBILE 1
#else
#define PLATFORM_MACOS 1
#define PLATFORM_DESKTOP 1
#endif
#elif defined(__ANDROID__)
#define PLATFORM_ANDROID 1
#define PLATFORM_MOBILE 1
#elif defined(__linux__)
#define PLATFORM_LINUX 1
#define PLATFORM_DESKTOP 1
#endif

#include "platform_types.h"
#include "platform_time.h"
#include "file_system.h"

#include "logging/log.h"

// Include platform-specific implementations
#if PLATFORM_DESKTOP
#include "desktop/desktop_display_surface.h"
#include "desktop/desktop_app_lifecycle.h"
#include "desktop/desktop_input_system.h"

namespace Core {
    using PlatformDisplaySurface = DesktopDisplaySurface;
    using PlatformAppLifecycle = DesktopAppLifecycle;
    using PlatformInputSystem = DesktopInputSystem;
}
#elif PLATFORM_ANDROID

#include "android/android_display_surface.h"
#include "android/android_app_lifecycle.h"
#include "android/android_input_system.h"

namespace Core {
    using PlatformDisplaySurface = AndroidDisplaySurface;
    using PlatformAppLifecycle = AndroidAppLifecycle;
    using PlatformInputSystem = AndroidInputSystem;
}
#endif

namespace Core {
    class Platform {
        public:
        static Platform& Get() {
            static Platform instance;
            return instance;
        }
        struct Config {
            int displayWidth = 1920;
            int displayHeight = 1080;
            const char* appName = "Application";
            bool enableValidation = false;

            //logger config
            const char* logFilename = "engine.log";
            bool logToConsole = true;
            bool logToFile = false;
            bool overwriteLog = true;
            EngineLogLevel logLevel = EngineLogLevel::Info;
            
            #if defined(__ANDROID__)
            // Android-specific parameters
            struct android_app* androidApp = nullptr;
            #endif
        };

        bool Initialize(const Config& config) {
            if(m_initialized) {
                return true;
            }
            if(!LogBackend::isInitialized()) {
                LogConfig logConfig{
                    .filename = config.logFilename,
                    .logToConsole = config.logToConsole,
                    .logToFile = config.logToFile,
                    .overwriteFile = config.overwriteLog,
                    .logLevelFilter = config.logLevel
                };

                if(!LogBackend::initialize(logConfig)) {
                    return false;
                }
            }

#if PLATFORM_DESKTOP
            glfwSetErrorCallback([](int error, const char* description) {
                LOG_ERROR("GLFW Error {}: {}", error, description);
            });

            if(!glfwInit()) {
                LOG_ERROR("Failed to initialize GLFW");
                return false;
            }

            if(!m_display.Initialize(config.displayWidth, config.displayHeight, config.appName)) {
                LOG_ERROR("Failed to create display surface");
                glfwTerminate();
                return false;
            }

            m_lifecycle.Initialize(m_display.GetBackendHandle());
            m_input.Initialize(m_display.GetBackendHandle());

            Time::Initialize();

            LOG_INFO("Platform initialized: {}", GetPlatformName());
            m_initialized = true;
            return true;
#elif PLATFORM_ANDROID
            if(!config.androidApp) {
                LOG_ERROR("Android app parameter is required for initialization");
                return false;
            }
            
            m_androidApp = config.androidApp;
            
            Time::Initialize();
            
            // Android subsystems are initialized, but don't set window yet
            // Window will be set via SetWindow() when APP_CMD_INIT_WINDOW arrives
            
            LOG_INFO("Platform initialized: {}", GetPlatformName());
            m_initialized = true;
            return true;
#else
            return false;
#endif
        }

        void Shutdown() {
            if(!m_initialized) {
                return;
            }

#if PLATFORM_DESKTOP
            m_input.Shutdown();
            m_display.Shutdown();

            glfwTerminate();

            LOG_INFO("Platform shut down");
            m_initialized = false;
#elif PLATFORM_ANDROID
            // Clear Android state
            m_androidApp = nullptr;
            
            LOG_INFO("Platform shut down");
            m_initialized = false;
#endif
        }

        PlatformDisplaySurface& GetDisplay() {
            return m_display;
        }
        PlatformAppLifecycle& GetLifecycle() {
            return m_lifecycle;
        }
        PlatformInputSystem& GetInput() {
            return m_input;
        }

        void ProcessEvents() {
#if PLATFORM_DESKTOP
            glfwPollEvents();
            m_lifecycle.Update();
#endif
        }

        static constexpr bool IsDesktop() {
#if PLATFORM_DESKTOP
            return true;
#else
            return false;
#endif
        }

        static constexpr bool IsMobile() {
#if PLATFORM_MOBILE
            return true;
#else
            return false;
#endif
        }

        static constexpr bool IsAndroid() {
#if PLATFORM_ANDROID
            return true;
#else
            return false;
#endif
        }

        static const char* GetPlatformName() {
#if PLATFORM_WINDOWS
            return "Windows";
#elif PLATFORM_MACOS
            return "macOS";
#elif PLATFORM_LINUX
            return "Linux";
#elif PLATFORM_ANDROID
            return "Android";
#elif PLATFORM_IOS
            return "iOS";
#else
            return "Unknown";
#endif
        }

        private:
        Platform() = default;
        ~Platform() {
            Shutdown();
        }

        Platform(const Platform&) = delete;
        Platform& operator=(const Platform&) = delete;

        PlatformDisplaySurface m_display;
        PlatformAppLifecycle m_lifecycle;
        PlatformInputSystem m_input;
        bool m_initialized = false;
        
        #if defined(__ANDROID__)
        struct android_app* m_androidApp = nullptr;
        #endif
    };

    inline PlatformDisplaySurface& Display() {
        return Platform::Get().GetDisplay();
    }
    inline PlatformAppLifecycle& Lifecycle() {
        return Platform::Get().GetLifecycle();
    }
    inline PlatformInputSystem& Input() {
        return Platform::Get().GetInput();
    }

} // namespace Core

using Key = Core::Key;
using MouseButton = Core::MouseButton;

class Input {
public:
    static bool GetKey(Key key) { 
        return Core::Platform::Get().GetInput().IsKeyPressed(key); 
    }
    
    static bool GetKeyDown(Key key) { 
        return Core::Platform::Get().GetInput().IsKeyJustPressed(key); 
    }
    
    static bool GetKeyUp(Key key) { 
        return Core::Platform::Get().GetInput().IsKeyJustReleased(key); 
    }
    
    static bool GetMouseButton(MouseButton button) {
        return Core::Platform::Get().GetInput().IsPointerDown(static_cast<int>(button));
    }
    
    static glm::vec2 GetMouseDelta() {
        return Core::Platform::Get().GetInput().GetPointerDelta(0);
    }
    
    static glm::vec2 GetMousePosition() {
        return Core::Platform::Get().GetInput().GetPointerPosition(0);
    }
    
    static glm::vec2 GetScrollDelta() {
        return Core::Platform::Get().GetInput().GetScrollDelta();
    }
};