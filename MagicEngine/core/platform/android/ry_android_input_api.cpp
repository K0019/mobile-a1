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
        static constexpr int MAX_PUMP_POINTERS = 5;
        static float lastx[MAX_PUMP_POINTERS] = {-1.f,-1.f,-1.f,-1.f,-1.f};
        static float lasty[MAX_PUMP_POINTERS] = {-1.f,-1.f,-1.f,-1.f,-1.f};

        for (int pid = 0; pid < MAX_PUMP_POINTERS; ++pid) {
            // DOWN
            if (g_cb_touch && g_input.IsPointerJustPressed(pid)) {
                auto p = g_input.GetPointerPosition(pid);
                g_cb_touch(p.x, p.y, pid, RY_TOUCH_DOWN);
            }

            // MOVE
            if (g_cb_touch && g_input.IsPointerDown(pid)) {
                auto p = g_input.GetPointerPosition(pid);
                if (p.x != lastx[pid] || p.y != lasty[pid]) {
                    g_cb_touch(p.x, p.y, pid, RY_TOUCH_MOVE);
                    lastx[pid] = p.x; lasty[pid] = p.y;
                }
            }

            // UP
            if (g_input.IsPointerJustReleased(pid)) {
                auto p = g_input.GetPointerPosition(pid);
                if (g_cb_touch) {
                    g_cb_touch(p.x, p.y, pid, RY_TOUCH_UP);
                    lastx[pid] = lasty[pid] = -1.f;
                }
                else if (g_cb_tap) {
                    g_cb_tap(p.x, p.y, pid);
                }
            }
        }

        g_input.Update();
    }



} // extern "C"
#endif // __ANDROID__
