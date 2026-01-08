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

	// Call once per frame after polling input
	void ry_pump_touch_events(void);
	int  ry_handle_ainput_event(void* ainput_event);

	//======================================================== Keep old tap callback as backup
	typedef void (*RyTapFn)(float x, float y, int pointerId);
	void ry_set_tap_callback(RyTapFn fn);
	//======================================================== 


#ifdef __cplusplus
} // extern "C"
#endif
