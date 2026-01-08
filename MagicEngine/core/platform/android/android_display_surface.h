// platform/android/android_display_surface.h
#pragma once

#include "../platform_types.h"
#include <android/native_window.h>
#include <functional>

namespace Core {

class AndroidDisplaySurface {
public:
    AndroidDisplaySurface() = default;
    ~AndroidDisplaySurface() { 
        if (m_window) {
            ANativeWindow_release(m_window);
        }
    }
    
    void SetWindow(ANativeWindow* window) {
        if (m_window == window) return;
        
        if (m_window) {
            if (onSurfaceDestroyed) {
                onSurfaceDestroyed();
            }
            ANativeWindow_release(m_window);
        }
        
        m_window = window;
        
        if (m_window) {
            ANativeWindow_acquire(m_window);
            m_width = ANativeWindow_getWidth(m_window);
            m_height = ANativeWindow_getHeight(m_window);
            
            if (onSurfaceCreated) {
                onSurfaceCreated();
            }
        } else {
            m_width = 0;
            m_height = 0;
        }
    }
    
    void UpdateDimensions() {
        if (!m_window) return;
        
        int newWidth = ANativeWindow_getWidth(m_window);
        int newHeight = ANativeWindow_getHeight(m_window);
        
        if (newWidth != m_width || newHeight != m_height) {
            m_width = newWidth;
            m_height = newHeight;
            
            if (onResize) {
                onResize(m_width, m_height);
            }
        }
    }
    
    bool IsValid() const { return m_window != nullptr; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    ANativeWindow* GetBackendHandle() const { return m_window; }
    void* GetVulkanWindowHandle() const { return m_window; }
    void* GetVulkanDisplayHandle() const { return nullptr; }
    
    std::function<void()> onSurfaceCreated;
    std::function<void()> onSurfaceDestroyed;
    std::function<void(int, int)> onResize;
    
private:
    ANativeWindow* m_window = nullptr;
    int m_width = 0;
    int m_height = 0;
};

} // namespace Core