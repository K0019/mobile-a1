#if defined(__ANDROID__)

#include "core/platform/android/android_input_system.h"   // your class
#include <android/input.h>
#include <android/log.h>
#include "core/platform/android/ry_android_input_api.h"
static Core::AndroidInputSystem g_input;
static RyTapFn g_cb_tap = nullptr;
static RyTouchCb g_cb_touch = nullptr;   // new (down/move/up)


extern "C" {

    void ry_set_tap_callback(RyTapFn fn) { g_cb_tap = fn; } //prev name was ry_fire_tap_if_any, keeping it as backup
    void ry_set_touch_callback(RyTouchCb f) { g_cb_touch = f; }

    int ry_handle_ainput_event(void* ainput_event) {
        return g_input.HandleInputEvent(reinterpret_cast<AInputEvent*>(ainput_event));
    }

    // Pump once per frame 
    void ry_pump_touch_events(void) {
        const int pid = 0;

        // DOWN
        if (g_cb_touch && g_input.IsPointerJustPressed(pid)) {
            auto p = g_input.GetPointerPosition(pid);
            g_cb_touch(p.x, p.y, pid, RY_TOUCH_DOWN);
        }

        // MOVE (basic: emit when position changes while held)
        static float lastx = -1.f, lasty = -1.f;
        if (g_cb_touch && g_input.IsPointerDown(pid)) {
            auto p = g_input.GetPointerPosition(pid);
            if (p.x != lastx || p.y != lasty) {
                g_cb_touch(p.x, p.y, pid, RY_TOUCH_MOVE);
                lastx = p.x; lasty = p.y;
            }
        }

        // UP
        if (g_input.IsPointerJustReleased(pid)) {
            auto p = g_input.GetPointerPosition(pid);

            if (g_cb_touch) {
                g_cb_touch(p.x, p.y, pid, RY_TOUCH_UP);
                lastx = lasty = -1.f;
              //  __android_log_print(ANDROID_LOG_INFO, "ry", "CB TOUCH (%.1f,%.1f)", p.x, p.y);
            }
            else if (g_cb_tap) {
                //For some reason this is nvr call or use, but for now keep it as spare backup as this was how it used to be done
                g_cb_tap(p.x, p.y, pid);
              //  __android_log_print(ANDROID_LOG_INFO, "ry", "CB TAP (%.1f,%.1f)", p.x, p.y);
            }
        }

        // Advance edge state at end of frame
        g_input.Update();
    }



} // extern "C"
#endif // __ANDROID__
