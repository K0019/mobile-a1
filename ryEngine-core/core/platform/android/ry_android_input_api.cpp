#if defined(__ANDROID__)

#include "core/platform/android/android_input_system.h"   // your class
#include <android/input.h>
#include <android/log.h>
#include "core/platform/android/ry_android_input_api.h"
static Core::AndroidInputSystem g_input;
static RyTapFn g_cb = nullptr;

extern "C" {

    void ry_set_tap_callback(RyTapFn fn) {
        g_cb = fn;
        __android_log_print(ANDROID_LOG_INFO, "ry", "cb=set? %s", fn ? "yes" : "no");
    }

    int ry_handle_ainput_event(void* ainput_event) {
        return g_input.HandleInputEvent(reinterpret_cast<AInputEvent*>(ainput_event));
    }

    void ry_tick_android_input(void) {
        g_input.Update(); // rolls justPressed/justReleased

        //if (!g_cb) return;

        //const int pid = 0;
        //if (g_input.IsPointerJustReleased(pid)) {
        //    const auto p = g_input.GetPointerPosition(pid);
        //    __android_log_print(ANDROID_LOG_INFO, "ry", "UP (%.1f,%.1f)", p.x, p.y);
        //    g_cb(p.x, p.y, pid);   // <-- this is the only thing Magic hooks
        //}
    }

    void ry_fire_tap_if_any(void) {
        const int pid = 0;

        // detect edges BEFORE tick/update
        if (g_input.IsPointerJustReleased(pid)) {
            auto p = g_input.GetPointerPosition(pid);
            __android_log_print(ANDROID_LOG_INFO, "ry", "[tap] (%.1f,%.1f)", p.x, p.y);
            if (g_cb) g_cb(p.x, p.y, pid);            // <-- hand it to Magic
        }

        // keep ry’s edge machine advancing once per frame
        g_input.Update();
    }

} // extern "C"
#endif // __ANDROID__
