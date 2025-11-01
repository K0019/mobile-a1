//#include <android/log.h>
//#include <android/android_native_app_glue.h>
//#include "platform/android/android_input_system.h"  // your wrapper
//
//static Core::AndroidInputSystem g_input;
//static int32_t(*sPrevOnInputEvent)(android_app*, AInputEvent*) = nullptr;
//
//static int32_t QuickTest_OnInput(android_app* app, AInputEvent* e) {
//    // Feed every input into your wrapper
//    int32_t handled = g_input.HandleInputEvent(e);
//    if (!handled && sPrevOnInputEvent) return sPrevOnInputEvent(app, e);
//    return handled;
//}
//
//namespace Core {
//
//    void AndroidInputQuickTest_Install(android_app* app) {
//        sPrevOnInputEvent = app->onInputEvent;
//        app->onInputEvent = QuickTest_OnInput;
//    }
//
//    void AndroidInputQuickTest_OnFrame() {
//        g_input.Update();
//        if (g_input.IsPointerJustReleased(0)) {
//            auto p = g_input.GetPointerPosition(0);
//            __android_log_print(ANDROID_LOG_INFO, "ry_quicktest",
//                "Tap @ (%.1f, %.1f)", p.x, p.y);
//        }
//    }
//
//    void AndroidInputQuickTest_Shutdown() {}
//
//} // namespace Core
//#endif // __ANDROID__