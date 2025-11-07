// platform/android/android_input_system.h
#pragma once

#include "../platform_types.h"
#include <android/input.h>
#include <glm/glm.hpp>
#include <array>

namespace Core {

class AndroidInputSystem {
public:
    static constexpr int MAX_KEYS_OF_DESKTOP = 512;   // match desktop

    AndroidInputSystem() {
        m_touches.fill(TouchState{});
        m_keyStates.fill(false);
        m_prevKeyStates.fill(false);
    }
    
    void Update() {
        m_prevKeyStates = m_keyStates;
        
        for (int i = 0; i < 3; ++i) {
            m_prevPointerStates[i] = m_pointerStates[i];
        }
        
        for (auto& touch : m_touches) {
            touch.justPressed = false;
            touch.justReleased = false;
        }
    }
    
    // Keyboard
    bool IsKeyPressed(Key key) const {
        int idx = static_cast<int>(key);
        return idx >= 0 && idx < MAX_KEYS_OF_DESKTOP && m_keyStates[idx];
    }
    
    bool IsKeyJustPressed(Key key) const {
        int idx = static_cast<int>(key);
        if (idx < 0 || idx >= MAX_KEYS_OF_DESKTOP) return false;
        return m_keyStates[idx] && !m_prevKeyStates[idx];
    }
    
    bool IsKeyJustReleased(Key key) const {
        int idx = static_cast<int>(key);
        if (idx < 0 || idx >= MAX_KEYS_OF_DESKTOP) return false;
        return !m_keyStates[idx] && m_prevKeyStates[idx];
    }
    
    int GetKeyModifiers() const { return 0; }
    
    // Pointer/Mouse (simulated from first touch)
    bool IsPointerDown(int index = 0) const {
        return index >= 0 && index < 3 && m_pointerStates[index];
    }
    
    bool IsPointerJustPressed(int index) const {
        return index >= 0 && index < 3 && 
               m_pointerStates[index] && !m_prevPointerStates[index];
    }
    
    bool IsPointerJustReleased(int index) const {
        return index >= 0 && index < 3 && 
               !m_pointerStates[index] && m_prevPointerStates[index];
    }
    
    glm::vec2 GetPointerPosition(int index = 0) const {
        if (index == 0 && m_touches[0].active) {
            return glm::vec2(m_touches[0].x, m_touches[0].y);
        }
        return glm::vec2(0.0f);
    }
    
    glm::vec2 GetPointerDelta(int index = 0) const {
        return glm::vec2(0.0f);
    }
    
    glm::vec2 GetScrollDelta() const {
        return glm::vec2(0.0f);
    }
    
    // Touch-specific interface
    bool IsTouchActive(int touchId) const {
        return touchId >= 0 && touchId < MAX_TOUCHES && m_touches[touchId].active;
    }
    
    glm::vec2 GetTouchPosition(int touchId) const {
        if (touchId >= 0 && touchId < MAX_TOUCHES) {
            return glm::vec2(m_touches[touchId].x, m_touches[touchId].y);
        }
        return glm::vec2(0.0f);
    }
    
    int GetActiveTouchCount() const {
        int count = 0;
        for (const auto& touch : m_touches) {
            if (touch.active) ++count;
        }
        return count;
    }
    
    // --- TK Interjection ---------------------------------------------

    void InjectKeyDown(Key key) { InjectKey(key, true); }
    void InjectKeyUp(Key key) { InjectKey(key, false); }
    void InjectKey(Key key, bool down) {
        int idx = static_cast<int>(key);
        if (idx < 0 || idx >= MAX_KEYS_OF_DESKTOP) return;
        m_keyStates[idx] = down;
        // Edge detection is handled by Update(): it compares m_keyStates vs m_prevKeyStates
    }

    void InjectPointerMove(float x, float y, int pointerIndex = 0) {
        if (pointerIndex < 0 || pointerIndex >= MAX_POINTERS) return;
        // Keep touch 0 and pointer 0 in sync so getters behave
        if (pointerIndex == 0) {
            m_touches[0].x = x;
            m_touches[0].y = y;
        }
        auto& p = m_pointerStates;
        auto& t = m_touches[std::min(pointerIndex, MAX_TOUCHES - 1)];
        t.x = x; t.y = y;
    }

    void InjectPointerDown(float x, float y, int pointerIndex = 0) {
        if (pointerIndex < 0 || pointerIndex >= MAX_POINTERS) return;
        if (pointerIndex == 0) {
            m_touches[0].x = x; m_touches[0].y = y;
            m_touches[0].active = true;
            // (justPressed edge will be inferred by Update via m_prevPointerStates)
            m_pointerStates[0] = true;
        }
        else {
            int id = std::min(pointerIndex, MAX_TOUCHES - 1);
            m_touches[id].x = x; m_touches[id].y = y;
            m_touches[id].active = true;
            // If you want pointer 1..2 to mirror, uncomment if used:
            // m_pointerStates[pointerIndex] = true;
        }
    }

    void InjectPointerUp(float x, float y, int pointerIndex = 0) {
        if (pointerIndex < 0 || pointerIndex >= MAX_POINTERS) return;
        if (pointerIndex == 0) {
            m_touches[0].x = x; m_touches[0].y = y;
            m_touches[0].active = false;
            m_pointerStates[0] = false;
        }
        else {
            int id = std::min(pointerIndex, MAX_TOUCHES - 1);
            m_touches[id].x = x; m_touches[id].y = y;
            m_touches[id].active = false;
            // m_pointerStates[pointerIndex] = false;
        }
    }
    // --- End Interjection -----------------------------------------------

    // Android event handling
    int32_t HandleInputEvent(AInputEvent* event) {
        int32_t eventType = AInputEvent_getType(event);
        
        if (eventType == AINPUT_EVENT_TYPE_MOTION) {
            return ProcessTouchEvent(event);
        } else if (eventType == AINPUT_EVENT_TYPE_KEY) {
            return ProcessKeyEvent(event);
        }
        
        return 0;
    }
    
private:
    struct TouchState {
        float x = 0.0f;
        float y = 0.0f;
        bool active = false;
        bool justPressed = false;
        bool justReleased = false;
    };
    
    static constexpr int MAX_TOUCHES = 10;
    static constexpr int MAX_POINTERS = 3;
    
    std::array<TouchState, MAX_TOUCHES> m_touches;
    std::array<bool, MAX_KEYS_OF_DESKTOP> m_keyStates{};
    std::array<bool, MAX_KEYS_OF_DESKTOP> m_prevKeyStates{};
    std::array<bool, MAX_POINTERS> m_pointerStates{};
    std::array<bool, MAX_POINTERS> m_prevPointerStates{};

    int32_t ProcessTouchEvent(AInputEvent* event) {
        int32_t action = AMotionEvent_getAction(event);
        int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
        size_t  pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
            >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

        int32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
        if (pointerId >= MAX_TOUCHES) return 0;

        float x = AMotionEvent_getX(event, pointerIndex);
        float y = AMotionEvent_getY(event, pointerIndex);

        TouchState& touch = m_touches[pointerId];

        // Index for LeftShift in your key state table
        const int SHIFT_IDX = static_cast<int>(Key::LeftShift);

        switch (actionMasked) {
        case AMOTION_EVENT_ACTION_DOWN:
        case AMOTION_EVENT_ACTION_POINTER_DOWN:
            touch.x = x;
            touch.y = y;
            touch.active = true;
            touch.justPressed = true;

            if (pointerId == 0) {
                m_pointerStates[0] = true;
            }

            // If this is the first active touch, press LeftShift
            {
                bool anyActive = false;
                for (const auto& t : m_touches) { if (t.active) { anyActive = true; break; } }
                if (anyActive) {
                    if (SHIFT_IDX >= 0 && SHIFT_IDX < MAX_KEYS_OF_DESKTOP) {
                        m_keyStates[SHIFT_IDX] = true;
                    }
                }
            }
            break;

        case AMOTION_EVENT_ACTION_MOVE:
            for (size_t i = 0; i < AMotionEvent_getPointerCount(event); ++i) {
                int32_t id = AMotionEvent_getPointerId(event, i);
                if (id >= MAX_TOUCHES) continue;

                TouchState& t = m_touches[id];
                if (t.active) {
                    t.x = AMotionEvent_getX(event, i);
                    t.y = AMotionEvent_getY(event, i);
                }
            }
            break;

        case AMOTION_EVENT_ACTION_UP:
        case AMOTION_EVENT_ACTION_POINTER_UP:
            touch.active = false;
            touch.justReleased = true;

            if (pointerId == 0) {
                m_pointerStates[0] = false;
            }

            // If there are no more active touches, release LeftShift
            {
                bool anyActive = false;
                for (const auto& t : m_touches) { if (t.active) { anyActive = true; break; } }
                if (!anyActive) {
                    if (SHIFT_IDX >= 0 && SHIFT_IDX < MAX_KEYS_OF_DESKTOP) {
                        m_keyStates[SHIFT_IDX] = false;
                    }
                }
            }
            break;

        case AMOTION_EVENT_ACTION_CANCEL:
            touch.active = false;
            touch.justReleased = true;

            // On cancel, also release pointer 0 if it was this one
            if (pointerId == 0) {
                m_pointerStates[0] = false;
            }

            // If no more active touches, release LeftShift
            {
                bool anyActive = false;
                for (const auto& t : m_touches) { if (t.active) { anyActive = true; break; } }
                if (!anyActive) {
                    if (SHIFT_IDX >= 0 && SHIFT_IDX < MAX_KEYS_OF_DESKTOP) {
                        m_keyStates[SHIFT_IDX] = false;
                    }
                }
            }
            break;
        }

        return 1;
    }
    
    //int32_t ProcessTouchEvent(AInputEvent* event) {
    //    int32_t action = AMotionEvent_getAction(event);
    //    int32_t actionMasked = action & AMOTION_EVENT_ACTION_MASK;
    //    size_t pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) 
    //                          >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    //    
    //    int32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
    //    if (pointerId >= MAX_TOUCHES) return 0;
    //    
    //    float x = AMotionEvent_getX(event, pointerIndex);
    //    float y = AMotionEvent_getY(event, pointerIndex);
    //    
    //    TouchState& touch = m_touches[pointerId];
    //    
    //    switch (actionMasked) {
    //        case AMOTION_EVENT_ACTION_DOWN:
    //        case AMOTION_EVENT_ACTION_POINTER_DOWN:
    //            touch.x = x;
    //            touch.y = y;
    //            touch.active = true;
    //            touch.justPressed = true;
    //            
    //            if (pointerId == 0) {
    //                m_pointerStates[0] = true;
    //            }
    //            break;
    //            
    //        case AMOTION_EVENT_ACTION_MOVE:
    //            for (size_t i = 0; i < AMotionEvent_getPointerCount(event); ++i) {
    //                int32_t id = AMotionEvent_getPointerId(event, i);
    //                if (id >= MAX_TOUCHES) continue;
    //                
    //                TouchState& t = m_touches[id];
    //                if (t.active) {
    //                    t.x = AMotionEvent_getX(event, i);
    //                    t.y = AMotionEvent_getY(event, i);
    //                }
    //            }
    //            break;
    //            
    //        case AMOTION_EVENT_ACTION_UP:
    //        case AMOTION_EVENT_ACTION_POINTER_UP:
    //            touch.active = false;
    //            touch.justReleased = true;
    //            
    //            if (pointerId == 0) {
    //                m_pointerStates[0] = false;
    //            }
    //            break;
    //            
    //        case AMOTION_EVENT_ACTION_CANCEL:
    //            touch.active = false;
    //            touch.justReleased = true;
    //            break;
    //    }
    //    
    //    return 1;
    //}
    
    int32_t ProcessKeyEvent(AInputEvent* event) {
        int32_t androidKey = AKeyEvent_getKeyCode(event);
        int32_t action = AKeyEvent_getAction(event);
        
        Key engineKey = AndroidKeyToEngineKey(androidKey);
        if (engineKey == Key::Unknown) return 0;
        
        int keyIndex = static_cast<int>(engineKey);
        if (keyIndex < 0 || keyIndex >= MAX_KEYS_OF_DESKTOP) return 0;
        
        m_keyStates[keyIndex] = (action == AKEY_EVENT_ACTION_DOWN);
        return 1;
    }
    
    Key AndroidKeyToEngineKey(int32_t androidKey) const {
        switch (androidKey) {
            case AKEYCODE_BACK: return Key::Escape;
            case AKEYCODE_SPACE: return Key::Space;
            case AKEYCODE_ENTER: return Key::Enter;
            case AKEYCODE_DEL: return Key::Backspace;
            case AKEYCODE_TAB: return Key::Tab;
            
            case AKEYCODE_DPAD_UP: return Key::Up;
            case AKEYCODE_DPAD_DOWN: return Key::Down;
            case AKEYCODE_DPAD_LEFT: return Key::Left;
            case AKEYCODE_DPAD_RIGHT: return Key::Right;
            
            case AKEYCODE_A: return Key::A;
            case AKEYCODE_B: return Key::B;
            case AKEYCODE_C: return Key::C;
            case AKEYCODE_D: return Key::D;
            case AKEYCODE_E: return Key::E;
            case AKEYCODE_F: return Key::F;
            case AKEYCODE_G: return Key::G;
            case AKEYCODE_H: return Key::H;
            case AKEYCODE_I: return Key::I;
            case AKEYCODE_J: return Key::J;
            case AKEYCODE_K: return Key::K;
            case AKEYCODE_L: return Key::L;
            case AKEYCODE_M: return Key::M;
            case AKEYCODE_N: return Key::N;
            case AKEYCODE_O: return Key::O;
            case AKEYCODE_P: return Key::P;
            case AKEYCODE_Q: return Key::Q;
            case AKEYCODE_R: return Key::R;
            case AKEYCODE_S: return Key::S;
            case AKEYCODE_T: return Key::T;
            case AKEYCODE_U: return Key::U;
            case AKEYCODE_V: return Key::V;
            case AKEYCODE_W: return Key::W;
            case AKEYCODE_X: return Key::X;
            case AKEYCODE_Y: return Key::Y;
            case AKEYCODE_Z: return Key::Z;
            
            case AKEYCODE_0: return Key::Num0;
            case AKEYCODE_1: return Key::Num1;
            case AKEYCODE_2: return Key::Num2;
            case AKEYCODE_3: return Key::Num3;
            case AKEYCODE_4: return Key::Num4;
            case AKEYCODE_5: return Key::Num5;
            case AKEYCODE_6: return Key::Num6;
            case AKEYCODE_7: return Key::Num7;
            case AKEYCODE_8: return Key::Num8;
            case AKEYCODE_9: return Key::Num9;
            
            default: return Key::Unknown;
        }
    }
};

} // namespace Core