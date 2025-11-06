#pragma once
#include "pch.h"

enum class TouchAction { Down = 0, Move = 1, Up = 2 };

struct TouchState {
	float x = 0.f, y = 0.f;
	bool down = false;
	bool justDown = false;
	bool justUp = false;
};

namespace AndroidInputBridge {
	void Initialize();                 // call once in MagicEngine::Init (Android only)
	const TouchState& State();         // read in your component
	void ClearEdges();                 // call once per frame after reading
}