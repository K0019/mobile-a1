#pragma once
#ifdef __cplusplus
extern "C" {
#endif
	// Callback Magic registers to receive pointer events.
	// action: 0=Down, 1=Up, 2=Move
	typedef void (*RyPointerFn)(int id, int action, float x, float y);

	void ry_input_set_pointer_callback(RyPointerFn fn);
	void ry_input_dispatch_frame_events(void);  // call once per frame (AFTER looper events, BEFORE Magic Update)

#ifdef __cplusplus
}
#endif
