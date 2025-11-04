#if defined(__ANDROID__)
#include "Engine/Platform/Android/MagicPointerInject.h"
#include "Engine/Input.h"            // KeyboardMouseInput, ST<>  :contentReference[oaicite:7]{index=7}
#include "Utilities/Logging.h"

//extern "C" void MagicInjectPointer(int pointerId, int action, float x, float y) {
//    auto* km = ST<KeyboardMouseInput>::Get();     // your input singleton user :contentReference[oaicite:8]{index=8}
//    // 1) Update position first
//    km->Platform_OnMouseMove(x, y);
//
//    // 2) Map actions to left mouse
//    if (action == 0) {
//        km->Platform_OnMouseDownLeft();
//    }
//    else if (action == 1) {
//        km->Platform_OnMouseUpLeft();
//    }
//    else {
//        // action == 2 (Move): nothing else to do
//    }
//
//    // Optional debug:
//    // CONSOLE_LOG(LEVEL_INFO) << "[Magic Inject] action="<<action<<" ("<<x<<","<<y<<") id="<<pointerId;
//}

//extern "C" {
//    void ry_set_pointer_callback(void (*fn)(int action, float x, float y, int pid));
//}
//
//// ryEngine -> Magic adapter
//static void Magic_OnPointerEvent(int ryAction, float x, float y, int pid)
//{
//    // Your MagicInjectPointer expects: 0=Down, 1=Up, 2=Move
//    switch (ryAction) {
//    case 0: MagicInjectPointer(pid, /*Move*/2, x, y); break;
//    case 1: MagicInjectPointer(pid, /*Down*/0, x, y); break;
//    case 2: MagicInjectPointer(pid, /*Up  */1, x, y); break;
//    default: break;
//    }
//}
//
//void Magic_AndroidInputBridge_Initialize()
//{
//    ry_set_pointer_callback(&Magic_OnPointerEvent);
//}
//

extern "C" void Magic_OnPointerEvent(float x, float y, int /*pointerId*/) {
    // For now, just log; later you can inject into Magic’s input
    CONSOLE_LOG(LEVEL_INFO) << "[Magic] tap (" << x << "," << y << ")";
    // If you have an injection API, call it here (press/release/mouse click).
}


#endif
