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
/*****************************************************************//*!
\brief
	Sets up the ryEngine touch callback(s) and resets internal state.
*//******************************************************************/
	void Initialize();                 // call once in MagicEngine::Init (Android only)


/*****************************************************************//*!
\brief
	Get the current debounced touch state for this frame.
\return
	 const TouchState&  Reference to the live state object.
*//******************************************************************/
	const TouchState& State();         // read in your component

/*****************************************************************//*!
\brief
	Clear one-frame edge flags after you’ve consumed the state.
*//******************************************************************/
	void ClearEdges();                 // call once per frame after reading (honestly not sure if needed tho)
}

namespace AndroidInputBridge {
	void PublishVirtualStick(const Vec2& v);  // called by your component
	Vec2  ReadVirtualStick();                  // used by Input system
}

// --- Exclusive touch capture -----------------------------------------------
enum class TouchOwner { NoOwner = 0, Joystick, UI, Camera };

// Returns current owner (for read-only checks).
TouchOwner Owner();

// Try to acquire exclusive touch ownership for this frame sequence.
// Returns true if granted (or already owned by the same owner).
bool TryCapture(TouchOwner who);

// Release if the caller currently owns the capture (no-op otherwise).
void Release(TouchOwner who);