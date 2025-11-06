#include "AndroidInputManager.h"

static inline float clampf(float v, float a, float b) { return v < a ? a : (v > b ? b : v); }
static std::string labelFromValue(const Vec2& v, float dead)
{
    std::string out;
    if (v.y > dead) out += 'W';
    else if (v.y < -dead) out += 'S';
    if (v.x < -dead) out += 'A';
    else if (v.x > dead) out += 'D';
    return out; // "", "W","A","D","S","WA","WD","SA","SD"
}
static inline bool inRect(const Vec2& p, const Vec2& mn, const Vec2& mx) {
    return (p.x >= mn.x && p.x <= mx.x && p.y >= mn.y && p.y <= mx.y);
}

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

    // ---- PRESS EDGE ----------------------------------------------------------
    if (t.justDown) {
        if (dynamicCenter) {
            // Only start if touch is inside [200..250] x [200..250]
            if (inRect(tp, dynZoneMin, dynZoneMax)) {
                m_center = tp;             // dynamic center under the finger
                m_active = true;
                CONSOLE_LOG(LEVEL_INFO)
                    << "[AIC] DOWN (dynamic-center) @ (" << tp.x << "," << tp.y
                    << "), center=(" << m_center.x << "," << m_center.y << ")";
            }
            else {
                m_active = false;          // ignore presses outside the zone
                CONSOLE_LOG(LEVEL_DEBUG)
                    << "[AIC] DOWN (ignored, outside dynamic zone) @ ("
                    << tp.x << "," << tp.y << ")";
            }
        }
        else {
            // Fixed-center mode: accept immediately (or add your capture-radius test here)
            m_active = true;
            CONSOLE_LOG(LEVEL_INFO)
                << "[AIC] DOWN (fixed-center) @ (" << tp.x << "," << tp.y << ")";
        }
    }

    // ---- held ---------------------------------------------------------------
    if (t.down && m_active) {
        const Vec2 d = tp - m_center;
        const float r = EffectiveRadius();                   // doubled radius
        const float dz = clampf(deadZoneFrac, 0.f, 0.9f) * r; // dead zone in px

        const float len2 = d.x * d.x + d.y * d.y;
        if (len2 <= 1e-6f) {
            m_value = Vec2{ 0.f, 0.f };
        }
        else {
            const float len = std::sqrt(len2);
            if (len <= dz) {
                m_value = Vec2{ 0.f, 0.f };
            }
            else {
                const float clamped = clampf(len, 0.f, r);
                const float usable = (clamped - dz) / (r - dz);   // [0..1]
                const Vec2  dirPhone = Vec2{ d.x / len, d.y / len };   // unit direction from touch
                // Swap X/Y to align with your screen orientation so W=up, D=right
                const Vec2  dirScreen = Vec2{ dirPhone.y, dirPhone.x }; 
                m_value = Vec2{ dirScreen.x * usable, dirScreen.y * usable };
            }
        }

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
        if (m_active)
            CONSOLE_LOG(LEVEL_DEBUG) << "[AIC] UP @ (" << tp.x << ", " << tp.y << ")";

        m_active = false;
        m_captured = false;
        m_value = Vec2{ 0.f, 0.f };
        m_prevDir.clear();
    }
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
