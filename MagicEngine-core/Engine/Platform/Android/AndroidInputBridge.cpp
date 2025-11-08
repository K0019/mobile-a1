/******************************************************************************/
/*!
\file   AndroidInputBridge.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This is the source file that contains the definition of the androidInputBridge.
      It helps track the touch state by getting ryEngine android input without needing
      android libraries here in MagicEngine

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#if defined(__ANDROID__)
#include "AndroidInputBridge.h"
#include "core/platform/android/ry_android_input_api.h"

// -----------------------------------------------------------------------------
// Single, static touch state used by the bridge.
// This is a "latest known state" snapshot updated by ryEngine's callback and
static TouchState s_touch;

/*****************************************************************//*!
    \brief
         RyEngine -> Magic Callback, call by the platform layer 
         whenever a touch action occurs
*//******************************************************************/
static void OnTouchFromRy(float x, float y, int /*id*/, ry_touch_action_t action) {
    //Update position to the latest
    s_touch.x = x; 
    s_touch.y = y;

    switch (action) {

    case RY_TOUCH_DOWN:
        if (!s_touch.down) 
            s_touch.justDown = true;
        s_touch.down = true;
      //  CONSOLE_LOG(LEVEL_DEBUG) << " RY_TOUCH_DOWN";

        break;
    case RY_TOUCH_MOVE:
        // position already updated
        s_touch.down = true;
     //   CONSOLE_LOG(LEVEL_DEBUG) << "RY_TOUCH_MOVE";

        break;
    case RY_TOUCH_UP:
        s_touch.down = false;
        s_touch.justUp = true;
    //    CONSOLE_LOG(LEVEL_DEBUG) << " RY_TOUCH_UP";

        break;
    }
}

namespace AndroidInputBridge {
    void Initialize() { 
        ry_set_touch_callback(&OnTouchFromRy);
        CONSOLE_LOG(LEVEL_DEBUG) << " AndroidInputBridge is initalised";
    }
    const TouchState& State() { return s_touch; }
    void ClearEdges() { s_touch.justDown = s_touch.justUp = false; }
}
#endif
