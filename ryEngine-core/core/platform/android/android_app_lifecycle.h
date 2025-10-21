// platform/android/android_app_lifecycle.h
#pragma once

#include "../platform_types.h"
#include <android/native_activity.h>
#include <functional>

namespace Core {

class AndroidAppLifecycle {
public:
    AndroidAppLifecycle() = default;
    
    void SetState(AppState state) {
        if (m_currentState == state) return;
        
        AppState oldState = m_currentState;
        m_currentState = state;
        
        if (onStateChange) {
            onStateChange(oldState, state);
        }
    }
    
    AppState GetState() const { return m_currentState; }
    bool ShouldQuit() const { return m_shouldQuit; }
    
    void NotifyDestroy() {
        m_shouldQuit = true;
    }
    
    // Android lifecycle events
    void OnResume() { SetState(AppState::Running); }
    void OnPause() { SetState(AppState::Paused); }
    void OnStop() { SetState(AppState::Stopped); }
    
    void OnLowMemory() {
        if (onLowMemory) {
            onLowMemory();
        }
    }
    
    std::function<void(AppState, AppState)> onStateChange;
    std::function<void()> onLowMemory;
    
private:
    AppState m_currentState = AppState::Created;
    bool m_shouldQuit = false;
};

} // namespace Core