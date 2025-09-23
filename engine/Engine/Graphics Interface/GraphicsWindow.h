#pragma once

class IGraphicsWindow
{
public:
	virtual void Init() = 0;

	virtual void SetPendingShutdown() = 0;
	virtual bool GetIsPendingShutdown() const = 0;

	virtual bool GetIsWindowMinimized() const = 0;

	virtual bool SetWindowIcon(const std::string& filepath) = 0;
	virtual void SetWindowResolution(int width, int height) = 0;
	virtual void SetFullscreen(bool isFullscreen) = 0;
	virtual void BringWindowToFront() = 0;

	// TODO: SetDragDropCallback
	// glfwSetDropCallback

public:
	// For compatibility with whatever old graphics interfaces are still here
	// Remove if possible once everything settles
	IntVec2 GetWindowExtent() const;
	IntVec2 GetViewportExtent() const;
	IntVec2 GetWorldExtent() const;

protected:
	IGraphicsWindow();

	// remove these at some point, kind of atrocious.
	IntVec2 windowExtent;
	IntVec2 viewportExtent;
	IntVec2 worldExtent;
};

class GraphicsWindowGLFW : public IGraphicsWindow
{
public:
	~GraphicsWindowGLFW();

	void Init() override;

	void SetPendingShutdown() override;
	bool GetIsPendingShutdown() const override;

	bool GetIsWindowMinimized() const override;

	bool SetWindowIcon(const std::string& filepath) override;
	void SetWindowResolution(int width, int height) override;
	void SetFullscreen(bool isFullscreen) override;
	void BringWindowToFront() override;

private:
	void InitGLFW();
	void SetupGLFWSettings();
	void SetupGLFWCallbacks();

	static void GLFW_Callback_FramebufferResize(GLFWwindow* window, int width, int height);
	void Callback_FramebufferResize(int width, int height);
	static void GLFW_Callback_WindowFocusChanged(GLFWwindow* window, int isFocused);
	static void GLFW_Callback_IconifyStateChanged(GLFWwindow* window, int isIconified);
	static void GLFW_Callback_CursorEnteredWindow(GLFWwindow* window, int isEntered);

public:
	GLFWwindow* INTERNAL_GetWindow();
	
private:
	GLFWwindow* window;
	GLFWmonitor* monitor;

};


using GraphicsWindow = GraphicsWindowGLFW;
