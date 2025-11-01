#if defined(__ANDROID__)

#include <android/input.h>                                   // Android lives ONLY here
#include "core/platform/android/android_input_system.h"      // your wrapper (implementation uses <android/*>)

// Static instance that owns touch/key state
static Core::AndroidInputSystem g_input;
static RyTapFn g_tap_cb = nullptr;

extern "C" {

    void ry_set_tap_callback(RyTapFn fn) {
        g_tap_cb = fn;
    }

    int ry_handle_ainput_event(void* ainput_event) {
        return g_input.HandleInputEvent(reinterpret_cast<AInputEvent*>(ainput_event));
    }

    void ry_tick_android_input(void) {
        g_input.Update();
        if (g_input.IsPointerJustReleased(0)) {
            const auto p = g_input.GetPointerPosition(0);
            if (g_tap_cb) g_tap_cb(p.x, p.y, /*pointerId*/0);
        }
    }

} // extern "C"
#endif // __ANDROID__
