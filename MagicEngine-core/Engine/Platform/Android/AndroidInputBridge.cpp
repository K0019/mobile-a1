
#if defined(__ANDROID__)
#include "AndroidInputBridge.h"
#include "core/platform/android/ry_android_input_api.h"

static TouchState s_touch;

static void OnTouchFromRy(float x, float y, int /*id*/, ry_touch_action_t action) {
    s_touch.x = x; s_touch.y = y;
    switch (action) {
    case RY_TOUCH_DOWN:
        if (!s_touch.down) s_touch.justDown = true;
        s_touch.down = true;
      //  CONSOLE_LOG(LEVEL_DEBUG) << " RY_TOUCH_DOWN";

        break;
    case RY_TOUCH_MOVE:
        // position already updated
        s_touch.down = true;
     //   CONSOLE_LOG(LEVEL_DEBUG) << "RY_TOUCH_MOVE";

        break;
    case RY_TOUCH_UP:
        s_touch.down = false;
        s_touch.justUp = true;
    //    CONSOLE_LOG(LEVEL_DEBUG) << " RY_TOUCH_UP";

        break;
    }
}

namespace AndroidInputBridge {
    void Initialize() { 
        ry_set_touch_callback(&OnTouchFromRy);
        CONSOLE_LOG(LEVEL_DEBUG) << " AndroidInputBridge is initalised";
    }
    const TouchState& State() { return s_touch; }
    void ClearEdges() { s_touch.justDown = s_touch.justUp = false; }
}
#endif
