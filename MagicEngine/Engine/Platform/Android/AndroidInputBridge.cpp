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

#include "AndroidInputBridge.h"
#include "core/platform/android/ry_android_input_api.h"

static constexpr int MP = AndroidInputBridge::MAX_POINTERS;
static TouchState s_touches[MP];
static TouchOwner s_owners[MP] = {};
static Vec2 s_virtualStick{ 0.f, 0.f };

#if defined(__ANDROID__)

static void OnTouchFromRy(float x, float y, int id, ry_touch_action_t action) {
    if (id < 0 || id >= MP) return;

    TouchState& t = s_touches[id];
    t.x = x;
    t.y = y;

    switch (action) {
    case RY_TOUCH_DOWN:
        if (!t.down)
            t.justDown = true;
        t.down = true;
        break;
    case RY_TOUCH_MOVE:
        t.down = true;
        break;
    case RY_TOUCH_UP:
        t.down = false;
        t.justUp = true;
        break;
    }
}

namespace AndroidInputBridge {
    void Initialize() {
        ry_set_touch_callback(&OnTouchFromRy);
        for (int i = 0; i < MP; ++i) {
            s_touches[i] = {};
            s_owners[i] = TouchOwner::NoOwner;
        }
    }

    const TouchState& State(int pointerId) {
        if (pointerId < 0 || pointerId >= MP) pointerId = 0;
        return s_touches[pointerId];
    }

    void ClearEdges() {
        for (int i = 0; i < MP; ++i) {
            s_touches[i].justDown = false;
            s_touches[i].justUp = false;
        }
    }

    void PublishVirtualStick(const Vec2& v) { s_virtualStick = v; }
    Vec2 ReadVirtualStick() { return s_virtualStick; }

    TouchOwner Owner(int pointerId) {
        if (pointerId < 0 || pointerId >= MP) return TouchOwner::NoOwner;
        return s_owners[pointerId];
    }

    bool TryCapture(TouchOwner who, int pointerId) {
        if (pointerId < 0 || pointerId >= MP) return false;
        if (s_owners[pointerId] == TouchOwner::NoOwner || s_owners[pointerId] == who) {
            s_owners[pointerId] = who;
            return true;
        }
        return false;
    }

    void Release(TouchOwner who) {
        for (int i = 0; i < MP; ++i) {
            if (s_owners[i] == who)
                s_owners[i] = TouchOwner::NoOwner;
        }
    }

    int GetOwnedPointer(TouchOwner who) {
        for (int i = 0; i < MP; ++i) {
            if (s_owners[i] == who) return i;
        }
        return -1;
    }

    int FindUnownedJustDown() {
        for (int i = 0; i < MP; ++i) {
            if (s_touches[i].justDown && s_owners[i] == TouchOwner::NoOwner)
                return i;
        }
        return -1;
    }
}

#else
// ---- non-Android stubs ----
namespace AndroidInputBridge {
    void Initialize() {}

    const TouchState& State(int pointerId) {
        static TouchState dummy;
        return dummy;
    }

    void ClearEdges() {}

    void PublishVirtualStick(const Vec2& v) { s_virtualStick = v; }
    Vec2 ReadVirtualStick() { return s_virtualStick; }

    TouchOwner Owner(int) { return TouchOwner::NoOwner; }
    bool TryCapture(TouchOwner, int) { return false; }
    void Release(TouchOwner) {}
    int GetOwnedPointer(TouchOwner) { return -1; }
    int FindUnownedJustDown() { return -1; }
}
#endif
