#pragma once
#ifdef __cplusplus
extern "C" {
#endif
	//For android use 
	typedef enum {
		RY_TOUCH_DOWN = 0,
		RY_TOUCH_MOVE = 1,
		RY_TOUCH_UP = 2
	} ry_touch_action_t;

	// New richer callback //TESTING FOR NOW
	typedef void (*RyTouchCb)(float x, float y, int pointerId, ry_touch_action_t action);
	void ry_set_touch_callback(RyTouchCb fn);


	// Keep old tap callback (fires only on release) for compatibility
	typedef void (*RyTapFn)(float x, float y, int pointerId);
	void ry_set_tap_callback(RyTapFn fn);


	// Call once per frame after polling input (replaces ry_fire_tap_if_any)
	void ry_pump_touch_events(void);

	int  ry_handle_ainput_event(void* ainput_event);
	//void ry_tick_android_input(void);
	//void ry_fire_tap_if_any(void);  // optional

#ifdef __cplusplus
} // extern "C"
#endif
