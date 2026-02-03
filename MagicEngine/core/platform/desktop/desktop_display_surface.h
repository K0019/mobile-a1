// platform/desktop/desktop_display_surface.h
#pragma once

#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#  define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#  if defined(WAYLAND)
#    define GLFW_EXPOSE_NATIVE_WAYLAND
#  else
#    define GLFW_EXPOSE_NATIVE_X11
#  endif
#elif __APPLE__
#  define GLFW_EXPOSE_NATIVE_COCOA
#else
#  error "Unsupported OS for GLFW native access"
#endif
#include "GLFW/glfw3.h"
#ifdef _WIN32
#  include <windows.h>
#  include "GLFW/glfw3native.h"
#elif defined(__linux__)
   // to fix later
#endif
#include <functional>

namespace Core {

	// Forward declaration
	class DesktopInputSystem;

	// Shared user data for GLFW callbacks - prevents multiple systems from overwriting each other
	struct GlfwWindowUserData {
		class DesktopDisplaySurface* displaySurface = nullptr;
		DesktopInputSystem* inputSystem = nullptr;
	};

	class DesktopDisplaySurface {
	public:
		DesktopDisplaySurface() = default;
		~DesktopDisplaySurface() { Shutdown(); }

		bool Initialize(int width, int height, const char* title) {
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
			glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
			glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

			m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
			if (!m_window) {
				return false;
			}

			m_width = width;
			m_height = height;
			m_isValid = true;

			// Use shared user data struct to allow multiple systems to coexist
			m_userData.displaySurface = this;
			glfwSetWindowUserPointer(m_window, &m_userData);
			glfwSetFramebufferSizeCallback(m_window, FramebufferResizeCallback);
			glfwSetWindowContentScaleCallback(m_window, ContentScaleCallback);

			return true;
		}

		// Get the shared user data for other systems to register themselves
		GlfwWindowUserData* GetUserData() { return &m_userData; }

		void Shutdown() {
			if (m_window) {
				glfwDestroyWindow(m_window);
				m_window = nullptr;
				m_isValid = false;
			}
		}

		void Show() {
			if (m_window) {
				glfwShowWindow(m_window);
			}
		}

		void Hide() {
			if (m_window) {
				glfwHideWindow(m_window);
			}
		}

		void setFullscreen(bool fullscreen) {
			if (!m_window) return;
			static int m_windowedX = 0;
			static int m_windowedY = 0;
			static int m_windowedWidth = 0;
			static int m_windowedHeight = 0;

			if (fullscreen && !isFullscreen()) {
				// Save current windowed state
				glfwGetWindowPos(m_window, &m_windowedX, &m_windowedY);
				glfwGetWindowSize(m_window, &m_windowedWidth, &m_windowedHeight);

				// Go fullscreen
				GLFWmonitor* monitor = glfwGetPrimaryMonitor();
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			}
			else if (!fullscreen && isFullscreen()) {
				// Restore windowed state
				glfwSetWindowMonitor(m_window, nullptr, m_windowedX, m_windowedY,
					m_windowedWidth, m_windowedHeight, 0);
			}
		}

		bool isFullscreen()
		{
			return m_window && glfwGetWindowMonitor(m_window) != nullptr;
		}


		int GetWidth() const { return m_width; }
		int GetHeight() const { return m_height; }
		bool IsValid() const { return m_isValid; }
		GLFWwindow* GetBackendHandle() const { return m_window; }

		// Get the DPI content scale factor for high-DPI displays
		float GetContentScale() const {
			if (!m_window) return 1.0f;
			float xscale = 1.0f, yscale = 1.0f;
			glfwGetWindowContentScale(m_window, &xscale, &yscale);
			return xscale; // Use x scale (typically same as y on most platforms)
		}

		// Add actual platform-specific handle extraction
		void* GetVulkanWindowHandle() const {
			if (!m_window) return nullptr;

#if defined(_WIN32)
			return glfwGetWin32Window(m_window);
#elif defined(__linux__)
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
			return glfwGetWaylandWindow(m_window);
#else
			return reinterpret_cast<void*>(glfwGetX11Window(m_window));
#endif
#elif defined(__APPLE__)
			return glfwGetCocoaWindow(m_window);
#else
			return nullptr;
#endif
		}

		void* GetVulkanDisplayHandle() const {
#if defined(__linux__) && !defined(VK_USE_PLATFORM_WAYLAND_KHR)
			return glfwGetX11Display();
#elif defined(__linux__) && defined(VK_USE_PLATFORM_WAYLAND_KHR)
			return glfwGetWaylandDisplay();
#else
			return nullptr;
#endif
		}

		std::function<void(int, int)> onResize;
		std::function<void(float)> onContentScaleChange;

	private:
		GLFWwindow* m_window = nullptr;
		int m_width = 0;
		int m_height = 0;
		bool m_isValid = false;
		GlfwWindowUserData m_userData;

		static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
			auto* userData = static_cast<GlfwWindowUserData*>(glfwGetWindowUserPointer(window));
			if (userData && userData->displaySurface) {
				auto* surface = userData->displaySurface;
				surface->m_width = width;
				surface->m_height = height;

				int iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
				surface->m_isValid = !(width == 0 && height == 0) && !iconified;

				if (surface->onResize) {
					surface->onResize(width, height);
				}
			}
		}

		static void ContentScaleCallback(GLFWwindow* window, float xscale, [[maybe_unused]] float yscale) {
			auto* userData = static_cast<GlfwWindowUserData*>(glfwGetWindowUserPointer(window));
			if (userData && userData->displaySurface) {
				auto* surface = userData->displaySurface;
				if (surface->onContentScaleChange) {
					surface->onContentScaleChange(xscale);
				}
			}
		}
	};

} // namespace Core
