// platform/desktop/desktop_input_system.h
#pragma once

#include "../platform_types.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <array>
#include <bitset>

namespace Core {

class DesktopInputSystem {
public:
    DesktopInputSystem() = default;
    
    void Initialize(GLFWwindow* window) {
        m_window = window;
        
        glfwSetWindowUserPointer(m_window, this);
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
        glfwSetCursorPosCallback(m_window, CursorPosCallback);
        glfwSetScrollCallback(m_window, ScrollCallback);
    }
    
    void Shutdown() {
        if (m_window) {
            glfwSetKeyCallback(m_window, nullptr);
            glfwSetMouseButtonCallback(m_window, nullptr);
            glfwSetCursorPosCallback(m_window, nullptr);
            glfwSetScrollCallback(m_window, nullptr);
            m_window = nullptr;
        }
    }
    
    void Update() {
        m_previousKeyStates = m_currentKeyStates;
        
        for (auto& pointer : m_pointers) {
            pointer.wasDown = pointer.isDown;
            pointer.previousPosition = pointer.position;
            pointer.delta = glm::vec2(0.0f);
        }
        
        m_scrollDelta = glm::vec2(0.0f);
    }
    
    bool IsKeyPressed(Key key) const {
        return m_currentKeyStates[static_cast<size_t>(key)];
    }
    
    bool IsKeyJustPressed(Key key) const {
        size_t idx = static_cast<size_t>(key);
        return m_currentKeyStates[idx] && !m_previousKeyStates[idx];
    }
    
    bool IsKeyJustReleased(Key key) const {
        size_t idx = static_cast<size_t>(key);
        return !m_currentKeyStates[idx] && m_previousKeyStates[idx];
    }
    
    int GetKeyModifiers() const { return m_keyModifiers; }
    
    bool IsPointerDown(int index = 0) const {
        return index >= 0 && index < MAX_POINTERS && m_pointers[index].isDown;
    }
    
    bool IsPointerJustPressed(int index) const {
        return index >= 0 && index < MAX_POINTERS && 
               m_pointers[index].isDown && !m_pointers[index].wasDown;
    }
    
    bool IsPointerJustReleased(int index) const {
        return index >= 0 && index < MAX_POINTERS && 
               !m_pointers[index].isDown && m_pointers[index].wasDown;
    }
    
    glm::vec2 GetPointerPosition(int index = 0) const {
        return (index >= 0 && index < MAX_POINTERS) ? m_pointers[index].position : glm::vec2(0.0f);
    }
    
    glm::vec2 GetPointerDelta(int index = 0) const {
        return (index >= 0 && index < MAX_POINTERS) ? m_pointers[index].delta : glm::vec2(0.0f);
    }
    
    glm::vec2 GetScrollDelta() const { return m_scrollDelta; }
    
    int GetActivePointerCount() const {
        int count = 0;
        for (const auto& pointer : m_pointers) {
            if (pointer.isDown) count++;
        }
        return count;
    }

private:
    static constexpr size_t MAX_KEYS = 512;
    static constexpr size_t MAX_POINTERS = 10;
    
    struct PointerState {
        glm::vec2 position{0.0f};
        glm::vec2 previousPosition{0.0f};
        glm::vec2 delta{0.0f};
        bool isDown = false;
        bool wasDown = false;
    };
    
    GLFWwindow* m_window = nullptr;
    std::bitset<MAX_KEYS> m_currentKeyStates;
    std::bitset<MAX_KEYS> m_previousKeyStates;
    std::array<PointerState, MAX_POINTERS> m_pointers;
    glm::vec2 m_scrollDelta{0.0f};
    int m_keyModifiers = 0;
    
    static void KeyCallback(GLFWwindow* window, int key, [[maybe_unused]] int scancode, int action, int mods) {
        auto* input = static_cast<DesktopInputSystem*>(glfwGetWindowUserPointer(window));
        if (!input) return;
        
        Key engineKey = GLFWKeyToEngineKey(key);
        size_t keyIndex = static_cast<size_t>(engineKey);
        if (keyIndex < MAX_KEYS) {
            input->m_currentKeyStates[keyIndex] = (action != GLFW_RELEASE);
            input->m_keyModifiers = GLFWModsToEngineMods(mods);
        }
    }
    
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, [[maybe_unused]] int mods) {
        auto* input = static_cast<DesktopInputSystem*>(glfwGetWindowUserPointer(window));
        if (!input) return;
        
        int pointerIndex = button;
        if (pointerIndex >= 0 && pointerIndex < MAX_POINTERS) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            
            input->m_pointers[pointerIndex].isDown = (action == GLFW_PRESS);
            input->m_pointers[pointerIndex].position = glm::vec2(static_cast<float>(xpos), static_cast<float>(ypos));
        }
    }
    
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        auto* input = static_cast<DesktopInputSystem*>(glfwGetWindowUserPointer(window));
        if (!input) return;
        
        glm::vec2 newPos(static_cast<float>(xpos), static_cast<float>(ypos));
        input->m_pointers[0].delta = newPos - input->m_pointers[0].position;
        input->m_pointers[0].position = newPos;
    }
    
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        auto* input = static_cast<DesktopInputSystem*>(glfwGetWindowUserPointer(window));
        if (!input) return;
        
        input->m_scrollDelta = glm::vec2(static_cast<float>(xoffset), static_cast<float>(yoffset));
    }
    
    static Key GLFWKeyToEngineKey(int glfwKey) {
        switch (glfwKey) {
            case GLFW_KEY_SPACE: return Key::Space;
            case GLFW_KEY_APOSTROPHE: return Key::Apostrophe;
            case GLFW_KEY_COMMA: return Key::Comma;
            case GLFW_KEY_MINUS: return Key::Minus;
            case GLFW_KEY_PERIOD: return Key::Period;
            case GLFW_KEY_SLASH: return Key::Slash;
            case GLFW_KEY_0: return Key::Num0;
            case GLFW_KEY_1: return Key::Num1;
            case GLFW_KEY_2: return Key::Num2;
            case GLFW_KEY_3: return Key::Num3;
            case GLFW_KEY_4: return Key::Num4;
            case GLFW_KEY_5: return Key::Num5;
            case GLFW_KEY_6: return Key::Num6;
            case GLFW_KEY_7: return Key::Num7;
            case GLFW_KEY_8: return Key::Num8;
            case GLFW_KEY_9: return Key::Num9;
            case GLFW_KEY_SEMICOLON: return Key::Semicolon;
            case GLFW_KEY_EQUAL: return Key::Equal;
            case GLFW_KEY_A: return Key::A;
            case GLFW_KEY_B: return Key::B;
            case GLFW_KEY_C: return Key::C;
            case GLFW_KEY_D: return Key::D;
            case GLFW_KEY_E: return Key::E;
            case GLFW_KEY_F: return Key::F;
            case GLFW_KEY_G: return Key::G;
            case GLFW_KEY_H: return Key::H;
            case GLFW_KEY_I: return Key::I;
            case GLFW_KEY_J: return Key::J;
            case GLFW_KEY_K: return Key::K;
            case GLFW_KEY_L: return Key::L;
            case GLFW_KEY_M: return Key::M;
            case GLFW_KEY_N: return Key::N;
            case GLFW_KEY_O: return Key::O;
            case GLFW_KEY_P: return Key::P;
            case GLFW_KEY_Q: return Key::Q;
            case GLFW_KEY_R: return Key::R;
            case GLFW_KEY_S: return Key::S;
            case GLFW_KEY_T: return Key::T;
            case GLFW_KEY_U: return Key::U;
            case GLFW_KEY_V: return Key::V;
            case GLFW_KEY_W: return Key::W;
            case GLFW_KEY_X: return Key::X;
            case GLFW_KEY_Y: return Key::Y;
            case GLFW_KEY_Z: return Key::Z;
            case GLFW_KEY_LEFT_BRACKET: return Key::LeftBracket;
            case GLFW_KEY_BACKSLASH: return Key::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return Key::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return Key::GraveAccent;
            case GLFW_KEY_ESCAPE: return Key::Escape;
            case GLFW_KEY_ENTER: return Key::Enter;
            case GLFW_KEY_TAB: return Key::Tab;
            case GLFW_KEY_BACKSPACE: return Key::Backspace;
            case GLFW_KEY_INSERT: return Key::Insert;
            case GLFW_KEY_DELETE: return Key::Delete;
            case GLFW_KEY_RIGHT: return Key::Right;
            case GLFW_KEY_LEFT: return Key::Left;
            case GLFW_KEY_DOWN: return Key::Down;
            case GLFW_KEY_UP: return Key::Up;
            case GLFW_KEY_PAGE_UP: return Key::PageUp;
            case GLFW_KEY_PAGE_DOWN: return Key::PageDown;
            case GLFW_KEY_HOME: return Key::Home;
            case GLFW_KEY_END: return Key::End;
            case GLFW_KEY_CAPS_LOCK: return Key::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return Key::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return Key::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return Key::PrintScreen;
            case GLFW_KEY_PAUSE: return Key::Pause;
            case GLFW_KEY_F1: return Key::F1;
            case GLFW_KEY_F2: return Key::F2;
            case GLFW_KEY_F3: return Key::F3;
            case GLFW_KEY_F4: return Key::F4;
            case GLFW_KEY_F5: return Key::F5;
            case GLFW_KEY_F6: return Key::F6;
            case GLFW_KEY_F7: return Key::F7;
            case GLFW_KEY_F8: return Key::F8;
            case GLFW_KEY_F9: return Key::F9;
            case GLFW_KEY_F10: return Key::F10;
            case GLFW_KEY_F11: return Key::F11;
            case GLFW_KEY_F12: return Key::F12;
            case GLFW_KEY_KP_0: return Key::Kp0;
            case GLFW_KEY_KP_1: return Key::Kp1;
            case GLFW_KEY_KP_2: return Key::Kp2;
            case GLFW_KEY_KP_3: return Key::Kp3;
            case GLFW_KEY_KP_4: return Key::Kp4;
            case GLFW_KEY_KP_5: return Key::Kp5;
            case GLFW_KEY_KP_6: return Key::Kp6;
            case GLFW_KEY_KP_7: return Key::Kp7;
            case GLFW_KEY_KP_8: return Key::Kp8;
            case GLFW_KEY_KP_9: return Key::Kp9;
            case GLFW_KEY_KP_DECIMAL: return Key::KpDecimal;
            case GLFW_KEY_KP_DIVIDE: return Key::KpDivide;
            case GLFW_KEY_KP_MULTIPLY: return Key::KpMultiply;
            case GLFW_KEY_KP_SUBTRACT: return Key::KpSubtract;
            case GLFW_KEY_KP_ADD: return Key::KpAdd;
            case GLFW_KEY_KP_ENTER: return Key::KpEnter;
            case GLFW_KEY_KP_EQUAL: return Key::KpEqual;
            case GLFW_KEY_LEFT_SHIFT: return Key::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return Key::LeftControl;
            case GLFW_KEY_LEFT_ALT: return Key::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return Key::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return Key::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return Key::RightControl;
            case GLFW_KEY_RIGHT_ALT: return Key::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return Key::RightSuper;
            case GLFW_KEY_MENU: return Key::Menu;
            default: return Key::Unknown;
        }
    }
    
    static int GLFWModsToEngineMods(int glfwMods) {
        int mods = 0;
        if (glfwMods & GLFW_MOD_SHIFT) mods |= KeyMod::Shift;
        if (glfwMods & GLFW_MOD_CONTROL) mods |= KeyMod::Control;
        if (glfwMods & GLFW_MOD_ALT) mods |= KeyMod::Alt;
        if (glfwMods & GLFW_MOD_SUPER) mods |= KeyMod::Super;
        if (glfwMods & GLFW_MOD_CAPS_LOCK) mods |= KeyMod::CapsLock;
        if (glfwMods & GLFW_MOD_NUM_LOCK) mods |= KeyMod::NumLock;
        return mods;
    }
};

} // namespace Core
