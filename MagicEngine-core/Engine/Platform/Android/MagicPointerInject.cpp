#if defined(__ANDROID__)
#include "Engine/Platform/Android/MagicPointerInject.h"
#include "Engine/Input.h"            // KeyboardMouseInput, ST<>  :contentReference[oaicite:7]{index=7}
#include "Utilities/Logging.h"

extern "C" void MagicInjectPointer(int pointerId, int action, float x, float y) {
    auto* km = ST<KeyboardMouseInput>::Get();     // your input singleton user :contentReference[oaicite:8]{index=8}
    // 1) Update position first
    km->Platform_OnMouseMove(x, y);

    // 2) Map actions to left mouse
    if (action == 0) {
        km->Platform_OnMouseDownLeft();
    }
    else if (action == 1) {
        km->Platform_OnMouseUpLeft();
    }
    else {
        // action == 2 (Move): nothing else to do
    }

    // Optional debug:
    // CONSOLE_LOG(LEVEL_INFO) << "[Magic Inject] action="<<action<<" ("<<x<<","<<y<<") id="<<pointerId;
}
#endif
