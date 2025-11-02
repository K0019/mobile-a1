#pragma once
#ifdef __cplusplus
extern "C" {
#endif

	typedef void (*RyTapFn)(float x, float y, int pointerId);

	void ry_set_tap_callback(RyTapFn fn);
	int  ry_handle_ainput_event(void* ainput_event);
	void ry_tick_android_input(void);
	void ry_fire_tap_if_any(void);  // optional

#ifdef __cplusplus
} // extern "C"
#endif
