// platform/desktop/desktop_app_lifecycle.h
#pragma once

#include "../platform_types.h"
#include <GLFW/glfw3.h>
#include <functional>

namespace Core {

class DesktopAppLifecycle {
public:
    DesktopAppLifecycle() = default;
    
    void Initialize(GLFWwindow* window) {
        m_window = window;
        m_currentState = AppState::Running;
        m_shouldQuit = false;

#ifdef NDEBUG
		// Locks mouse on release builds
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
#endif
    }
    
    void Update() {
        if (!m_window) return;
        
        if (glfwWindowShouldClose(m_window)) {
            if (m_currentState != AppState::Destroying) {
                SetState(AppState::Destroying);
                m_shouldQuit = true;
            }
            return;
        }
        
        int iconified = glfwGetWindowAttrib(m_window, GLFW_ICONIFIED);
        
        if (iconified && m_currentState == AppState::Running) {
            SetState(AppState::Paused);
        } else if (!iconified && m_currentState == AppState::Paused) {
            SetState(AppState::Running);
        }
    }
    
    void RequestExit() {
        if (m_window) {
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
            m_shouldQuit = true;
        }
    }
    
    AppState GetState() const { return m_currentState; }
    bool ShouldQuit() const { return m_shouldQuit; }
    bool IsInForeground() const { return m_currentState == AppState::Running; }
    
    std::function<void(AppState, AppState)> onStateChange;
    std::function<void()> onLowMemory;
    std::function<void()> onConfigurationChange;
    
private:
    GLFWwindow* m_window = nullptr;
    AppState m_currentState = AppState::Created;
    bool m_shouldQuit = false;
    
    void SetState(AppState newState) {
        if (newState != m_currentState) {
            AppState oldState = m_currentState;
            m_currentState = newState;
            if (onStateChange) {
                onStateChange(oldState, newState);
            }
        }
    }
};

} // namespace Core
