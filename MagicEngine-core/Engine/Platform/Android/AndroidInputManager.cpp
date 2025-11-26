/******************************************************************************/
/*!
\file   AndroidInputManager.cpp
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   26/09/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
      This is the source file that contains the definition of the AndroidInputManager.
      Currently it only handles a virtual joystick control.


All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/

#include "AndroidInputManager.h"

// ---------- Local helpers (internal linkage) --------------------------------

/*****************************************************************//*!
\brief
    Clamp a float into the closed interval [a, b].
\param v
    Input value
\param a
    Lower bound
\param b
    Upper bound
\return
    Clamped value in [a, b].
*//******************************************************************/
static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }

/*****************************************************************//*!
\brief
    Map a normalized joystick vector to "W/A/S/D" style labels
    Diagonals are combined (e.g., "WA", "WD", "SA", "SD")
\param v
    Normalized vector in [-1,1] (post dead-zone)
\param dead
    Dead threshold in normalized units; components with |comp| <= dead are off
\return
    Direction label: "", "W","A","S","D","WA","WD","SA","SD"
*//******************************************************************/
static std::string labelFromValue(const Vec2& v, float dead)
{
    std::string out;
    if (v.y > dead) out += 'W';
    else if (v.y < -dead) out += 'S';
    if (v.x < -dead) out += 'A';
    else if (v.x > dead) out += 'D';
    return out; // "", "W","A","D","S","WA","WD","SA","SD"
}

/*****************************************************************//*!
\brief
    Test whether a point lies inside (or on) an axis-aligned rectangle
\param p
    Point in screen space
\param mn
    Rectangle minimum 
\param mx
    Rectangle maximum
\return
    True if p is within [mn, mx], else false
*//******************************************************************/
static inline bool inRect(const Vec2& p, const Vec2& mn, const Vec2& mx) {
    return (p.x >= mn.x && p.x <= mx.x && p.y >= mn.y && p.y <= mx.y);
}

// -------------------------------------------------------------------------------


void AndroidInputComp::placeInitialCenter() {
    const float r = EffectiveRadius();
    const float margin = r + 20.0f; // keep the whole stick on screen
    m_center = Vec2{ margin, Core::Platform::Get().GetDisplay().GetHeight() - margin };
}

// Helpers
float AndroidInputComp::EffectiveRadius() const {
    return std::max(1.0f, baseRadius * radiusScale);
}
float AndroidInputComp::DeadZonePixels() const {
    const float f = clampf(deadZoneFrac, 0.0f, 0.9f);
    return f * EffectiveRadius();
}
Vec2 AndroidInputComp::PhoneToScreen(Vec2 p) const {
    const float W = (float)Core::Platform::Get().GetDisplay().GetWidth();
    const float H = (float)Core::Platform::Get().GetDisplay().GetHeight();

    switch (touchRotation) {
    case TouchRot::Rot0:      return { p.x, p.y };
    case TouchRot::Rot90CW:   return { H - p.y, p.x }; // phone (0,0) top-left -> screen rotated
    case TouchRot::Rot90CCW:  return { p.y, W - p.x };
    }
    return p;
}

AndroidInputComp::AndroidInputComp()
{
}

void AndroidInputComp::OnAttached()
{
    IGameComponentCallbacks::OnAttached();
}

void AndroidInputComp::OnStart()
{
    radiusScale = 1.0;
    m_value  = Vec2{ 0.f,0.f };
    dynamicCenter = true;
    if (dynamicCenter) {
        placeInitialCenter();
    }
    else {
        m_center = Vec2{ 250.f, 250.f };
        m_value = Vec2{ 0.f, 0.f };
    }
    m_active = false;
#if defined(__ANDROID__)
    CONSOLE_LOG(LEVEL_DEBUG) << "OnStart of AndroidInputComp Running";
#endif

#ifndef __ANDROID__
    ecs::DeleteEntity(ecs::GetEntity(this));
#endif
}

void AndroidInputComp::OnDetached()
{
}


// Read one-frame edge state (latched by the bridge earlier this frame).
// State() exposes:
//   - down:    finger is currently on screen (true while held)
//   - justDown:became pressed this frame (rising edge)
//   - justUp:  became released this frame (falling edge)
//   - x, y:    screen-space touch coordinates in pixels (origin: top-left, y grows downward)
//
// IMPORTANT: elsewhere (e.g., Engine::ExecuteFrame), call AndroidInputBridge::ClearEdges()
// ONCE per frame *after* all systems/components have consumed justDown/justUp.
void AndroidInputComp::Update() {
#if defined(__ANDROID__)
    const auto& t = AndroidInputBridge::State();
    const Vec2  tp = PhoneToScreen({ t.x, t.y });   // rotated to screen coords

    auto* entity = ecs::GetEntity(this);
    if (!entity) return;
    auto& tf = entity->GetTransform();

    // ---- PRESS EDGE ----------------------------------------------------------
    if (t.justDown) {

        if (dynamicCenter) {
            // Accept only if starting inside the activation rectangle
            if (inRect(tp, dynZoneMin, dynZoneMax)) {
                if (AndroidInputBridge::TryCapture(TouchOwner::Joystick)) {
                    m_anchorWorld = tf.GetWorldPosition();
                    m_center = tp;             // dynamic center under the finger
                    m_active = true;
                    CONSOLE_LOG(LEVEL_INFO)
                        << "[AIC] DOWN (dynamic-center) @ (" << tp.x << "," << tp.y
                        << "), center=(" << m_center.x << "," << m_center.y << ")";
                }
            }
            else {
                // ignore presses outside the zone
                m_active = false;          
                CONSOLE_LOG(LEVEL_DEBUG)
                    << "[AIC] DOWN (ignored, outside dynamic zone) @ ("
                    << tp.x << "," << tp.y << ")";
            }
        }
        else {
            // Fixed-center mode: take control immediately
            if (AndroidInputBridge::TryCapture(TouchOwner::Joystick)) {
                m_anchorWorld = tf.GetWorldPosition();
                m_active = true;
                CONSOLE_LOG(LEVEL_INFO)
                    << "[AIC] DOWN (fixed-center) @ (" << tp.x << "," << tp.y << ")";
            }
        }
    }

    // ---- Held ---------------------------------------------------------------
    if (t.down && m_active) {
        const Vec2 d = tp - m_center;                           // displacement from center (px)
        const float r = EffectiveRadius();                      // active  radius
        const float dz = clampf(deadZoneFrac, 0.f, 0.9f) * r;   // dead zone in px

        const float len2 = d.x * d.x + d.y * d.y;
        if (len2 <= 1e-6f) {
            // Essentially centered: output zero vector.
            m_value = Vec2{ 0.f, 0.f };
        }
        else {
            const float len = std::sqrt(len2);
            if (len <= dz) {
                // Inside dead-zone: no output.
                m_value = Vec2{ 0.f, 0.f };
            }
            else {
                // Outside dead-zone:
                //  - clamp distance to r
                //  - compute usable magnitude in [0..1] (dead-zone removed)
                //  - normalize direction and (optionally) swap axes to match labels
                const float clamped = clampf(len, 0.f, r);
                const float usable = (clamped - dz) / (r - dz);   // [0..1]

                const Vec2  dirPhone = Vec2{ d.x / len, d.y / len };   // unit direction from touch
                // Swap X/Y to align with your screen orientation so W=up, D=right
                const Vec2  dirScreen = Vec2{ dirPhone.y, dirPhone.x };
                m_value = Vec2{ dirScreen.x * usable, dirScreen.y * usable };
            }
        }

        //=====Start joystick Design ============================================================
        Vec3  pos = tf.GetWorldPosition();   // keep pos.z unchanged

        // sign flips if axis feels reversed
        const float sx = invertX ? -1.f : 1.f;
        const float sy = invertY ? -1.f : 1.f;

        // Build target within a circle of radius followWorld centered at anchor
        Vec2  offset = Vec2{ sx * m_value.x, sy * m_value.y } *followWorld;
        Vec2 m_anchorXY = Vec2{ m_anchorWorld.x , m_anchorWorld.y };
        Vec2  targetXY = m_anchorXY + offset;

        // Snap (hard clamp):
        tf.SetWorldPosition(Vec3{ targetXY.y,targetXY.x, pos.z });
        //=====End joystick Design ============================================================


        // Map to W/A/S/D label (with diagonals) and print only on change
        const std::string cur = labelFromValue(m_value, dirDead);
        if (cur != m_prevDir) {
            if (!cur.empty())
                CONSOLE_LOG(LEVEL_INFO) << "[AIC] DIR = " << cur << "  (v=" << m_value.x << "," << m_value.y << ")";
            m_prevDir = cur;
        }
    }

    // ---- release edge -------------------------------------------------------
    if (t.justUp) {
        if (m_active) {
            CONSOLE_LOG(LEVEL_DEBUG) << "[AIC] UP @ (" << tp.x << ", " << tp.y << ")";

            if (recenterOnRelease) {
                // Return to anchor so the object doesn't drift permanently
                tf.SetWorldPosition(m_anchorWorld); // snap back
            }
            // Give up exclusive ownership so camera/UI can use the next touch
            AndroidInputBridge::Release(TouchOwner::Joystick);
        }


        m_active = false;
        m_captured = false;
        m_value = Vec2{ 0.f, 0.f };
        m_prevDir.clear();
    }

    //let the AndroidInputComp push its [-1,1]^2 each frame, and the input system consumer pull it
    AndroidInputBridge::PublishVirtualStick(m_value);

#endif
}




void AndroidInputComp::EditorDraw()
{
}

AndroidInputSystem::AndroidInputSystem()
    : System_Internal{ &AndroidInputSystem::UpdateComp }
{
}

void AndroidInputSystem::UpdateComp(AndroidInputComp& comp)
{
    comp.Update();
}
