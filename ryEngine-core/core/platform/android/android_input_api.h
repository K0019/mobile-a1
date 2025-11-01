//#pragma once
//
//// Forward-declare to avoid pulling Android headers into non-Android builds.
//struct android_app;
//
//namespace Core {
//    // Install our input hook into app_glue (set onInputEvent)
//    void AndroidInputQuickTest_Install(android_app* app);
//
//    // Call once per frame, after you've pumped app_glue events.
//    void AndroidInputQuickTest_OnFrame();
//
//    // Optional: call on shutdown if you want (currently no-op).
//    void AndroidInputQuickTest_Shutdown();
//}