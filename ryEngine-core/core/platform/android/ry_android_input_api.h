#pragma once
#ifdef __cplusplus
extern "C" {
#endif

	// Simple callback Magic can install to get a tap (x,y). pointerId is optional for multi-touch.
	typedef void (*RyTapFn)(float x, float y, int pointerId);

	// Called once by Magic during init (Android build only).
	void ry_set_tap_callback(RyTapFn fn);

	// Call this from ry's Android input callback (where you already receive AInputEvent*).
	// The pointer stays opaque to Magic; only ry sees the real type in its .cpp.
	int  ry_handle_ainput_event(void* ainput_event);

	// Call once per frame (after draining events). If a tap happened, we invoke the callback.
	void ry_tick_android_input(void);

#ifdef __cplusplus
}
#endif