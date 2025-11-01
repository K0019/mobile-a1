#pragma once
#include "pch.h"
#ifdef __cplusplus
extern "C" {
#endif
	// action: 0 = Down, 1 = Up, 2 = Move
	void MagicInjectPointer(int pointerId, int action, float x, float y);
#ifdef __cplusplus
}
#endif
