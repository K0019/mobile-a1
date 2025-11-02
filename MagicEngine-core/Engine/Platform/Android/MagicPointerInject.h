#pragma once
#include "pch.h"
//#ifdef __cplusplus
//extern "C" {
//#endif
//	// action: 0 = Down, 1 = Up, 2 = Move
//	void MagicInjectPointer(int pointerId, int action, float x, float y);
//#ifdef __cplusplus
//}
//#endif

//#if defined(__ANDROID__)
//extern "C" {
//    void ry_set_pointer_event_callback(void (*fn)(int action, float x, float y, int pid));
//}
//
//void Magic_AndroidInputBridge_Initialize(); 
//
//#endif

extern "C" void Magic_OnPointerEvent(float x, float y, int pointerId);
