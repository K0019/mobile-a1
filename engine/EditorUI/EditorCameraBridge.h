// EditorCameraBridge.h
#pragma once
#include <glm/glm.hpp>

// A snapshot of the scene camera that editor code can read.
struct EditorCameraSnapshot {
    glm::mat4 view{ 1.0f };
    glm::mat4 proj{ 1.0f };
    bool orthographic{ false };
    bool valid{ false };
};

// Inline global so you don't need a .cpp or extern.
inline EditorCameraSnapshot g_EditorCamera;

// Producer: call this from your renderer when you have the camera matrices.
inline void EditorCam_Publish(const glm::mat4& V,
    const glm::mat4& P,
    bool ortho = false)
{
    g_EditorCamera.view = V;
    g_EditorCamera.proj = P;
    g_EditorCamera.orthographic = ortho;
    g_EditorCamera.valid = true;
}

// Consumer: called by ImGui/editor code to get the latest camera.
inline bool EditorCam_TryGet(glm::mat4& V,
    glm::mat4& P,
    bool& ortho)
{
    if (!g_EditorCamera.valid) return false;
    V = g_EditorCamera.view;
    P = g_EditorCamera.proj;
    ortho = g_EditorCamera.orthographic;
    return true;
}