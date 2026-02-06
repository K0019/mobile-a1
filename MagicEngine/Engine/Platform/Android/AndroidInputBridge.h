/******************************************************************************/
/*!
\file   AndroidInputBridge.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
	  This is the header file that contains the declaration of the androidInputBridge.
	  It helps track the touch state by getting ryEngine android input without needing
	  android libraries here in MagicEngine

	  TODO: Merge this into Input.h

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#pragma once
#include "pch.h"
#include <atomic>

 /*****************************************************************//*!
	 \brief
		  Discrete touch action used by the internal callback layer.
 *//******************************************************************/
enum class TouchAction { Down = 0, Move = 1, Up = 2 };

// --- Exclusive touch capture -----------------------------------------------
enum class TouchOwner { NoOwner = 0, Joystick, UI, Camera };

/*****************************************************************//*!
\brief
	Debounced per-frame touch state.
*//******************************************************************/
struct TouchState {
	float x = 0.f, y = 0.f;
	bool down = false;
	bool justDown = false;
	bool justUp = false;
};

namespace AndroidInputBridge {
    static constexpr int MAX_POINTERS = 5;

    void Initialize();

    const TouchState& State(int pointerId = 0);

    void ClearEdges();

    void PublishVirtualStick(const Vec2& v);
    Vec2 ReadVirtualStick();

    TouchOwner Owner(int pointerId = 0);

    bool TryCapture(TouchOwner who, int pointerId = 0);

    void Release(TouchOwner who);

    int  GetOwnedPointer(TouchOwner who);

    int  FindUnownedJustDown();
}
