#if defined(__ANDROID__)

#include "core/platform/android/android_input_system.h"   // your class
#include <android/input.h>
#include <android/log.h>
#include "core/platform/android/ry_android_input_api.h"
static Core::AndroidInputSystem g_input;
static RyTapFn g_cb_tap = nullptr;
static RyTouchCb g_cb_touch = nullptr;   // new (down/move/up)


extern "C" {

    void ry_set_tap_callback(RyTapFn fn) { g_cb_tap = fn; }
    void ry_set_touch_callback(RyTouchCb f) { g_cb_touch = f; }

    int ry_handle_ainput_event(void* ainput_event) {
        return g_input.HandleInputEvent(reinterpret_cast<AInputEvent*>(ainput_event));
    }

    // Pump once per frame (replace ry_fire_tap_if_any)
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

    ////void ry_set_tap_callback(RyTapFn fn) {
    ////    g_cb = fn;
    ////    __android_log_print(ANDROID_LOG_INFO, "ry", "cb=set? %s", fn ? "yes" : "no");
    ////}


    //int ry_handle_ainput_event(void* ainput_event) {
    //    return g_input.HandleInputEvent(reinterpret_cast<AInputEvent*>(ainput_event));
    //}

    //void ry_tick_android_input(void) {
    //    g_input.Update(); // rolls justPressed/justReleased

    //    //if (!g_cb) return;

    //    //const int pid = 0;
    //    //if (g_input.IsPointerJustReleased(pid)) {
    //    //    const auto p = g_input.GetPointerPosition(pid);
    //    //    __android_log_print(ANDROID_LOG_INFO, "ry", "UP (%.1f,%.1f)", p.x, p.y);
    //    //    g_cb(p.x, p.y, pid);   // <-- this is the only thing Magic hooks
    //    //}
    //}

    //void ry_fire_tap_if_any(void) {
    //    const int pid = 0;

    //    ///===== Not sure if needed==============================================
    //    //int w = Core::Platform::Get().GetDisplay().GetWidth();
    //    //int h = Core::Platform::Get().GetDisplay().GetHeight();

    //    //float engX = x;
    //    //float engY = (float)h - 1.0f - y;   // flip Y
    //    //// Normalized [0,1]
    //    //float u = x / (float)w;
    //    //float v = y / (float)h;        // or 1 - y/h if bottom-left origin

    //    //// NDC [-1,1] (OpenGL-style, y up)
    //    //float ndcX = 2.0f * x / w - 1.0f;
    //    //float ndcY = 1.0f - 2.0f * y / h;

    //    //// Ortho world (pixel-like world with camera at center)
    //    //worldX = (x - w * 0.5f) / zoom + cam.position.x;
    //    //worldY = (h * 0.5f - y) / zoom + cam.position.y;
    //    ///===== Not sure if needed==============================================


    //    // detect edges BEFORE tick/update
    //    if (g_input.IsPointerJustReleased(pid)) {
    //        auto p = g_input.GetPointerPosition(pid);
    //        __android_log_print(ANDROID_LOG_INFO, "ry", "[tap] (%.1f,%.1f)", p.x, p.y);
    //        if (g_cb) g_cb(p.x, p.y, pid);            // <-- hand it to Magic
    //    }

    //    // keep ry’s edge machine advancing once per frame
    //    g_input.Update();
    //}

} // extern "C"
#endif // __ANDROID__
