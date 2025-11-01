#if defined(__ANDROID__)
#include "core/platform/android/ry_android_input_api.h"
#include "core/platform/platform.h" // Platform::Get().GetInput()

static RyPointerFn g_cb = nullptr;

extern "C" {

    void ry_input_set_pointer_callback(RyPointerFn fn) { g_cb = fn; }

    void ry_input_dispatch_frame_events(void) {
        if (!g_cb) return;

        auto& in = Core::Platform::Get().GetInput();

        // Primary pointer id = 0 for now
        const int id = 0;
        const auto pos = in.GetPointerPosition(id);
        const auto d = in.GetPointerDelta(id);

        if (in.IsPointerJustPressed(id))   g_cb(id, 0, pos.x, pos.y);
        if (in.IsPointerDown(id) && (d.x != 0.f || d.y != 0.f)) g_cb(id, 2, pos.x, pos.y);
        if (in.IsPointerJustReleased(id))  g_cb(id, 1, pos.x, pos.y);
    }

} // extern "C"
#endif
