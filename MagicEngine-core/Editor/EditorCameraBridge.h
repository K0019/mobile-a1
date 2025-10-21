/******************************************************************************/
/*!
\file   EditorCameraBridge.h
\par    Project: KuroMahou
\par    Course: CSD3401
\par    Software Engineering Project 5
\date   10/10/2025

\author Hong Tze Keat (100%)
\par    email: h.tzekeat\@digipen.edu
\par    DigiPen login: h.tzekeat

\brief
    Declares a lightweight bridge for publishing and retrieving the scene
    camera (view/projection) so editor can render and hit-test 
    using the same matrices as renderer

All content © 2025 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include <glm/glm.hpp>

/*****************************************************************//*!
\struct EditorCameraSnapshot
\brief
  storing the camera matrices for the current frame.
*//******************************************************************/
struct EditorCameraSnapshot {
    glm::mat4 view{ 1.0f };
    glm::mat4 proj{ 1.0f };
    bool orthographic{ false };
    bool valid{ false }; // True if a snapshot has been published this frame
};

// Inline global so you don't need a .cpp or extern for current frame
inline EditorCameraSnapshot g_EditorCamera;

/*****************************************************************//*!
\brief
  Producer API: publish the curr frame camera matrices.

\param V
  View matrix
\param P
  Projection matrix (renderer’s NDC)
\param ortho
  check  camera is orthographic
*//******************************************************************/
inline void EditorCam_Publish(const glm::mat4& V,
    const glm::mat4& P,
    bool ortho = false)
{
    g_EditorCamera.view = V;
    g_EditorCamera.proj = P;
    g_EditorCamera.orthographic = ortho;
    g_EditorCamera.valid = true;
}

/*****************************************************************//*!
\brief
  Consumer API: retrieve the most recently published camera matrices

\param V
  View matrix
\param P
  Projection matrix (renderer’s NDC)
\param ortho
  check  camera is orthographic
*//******************************************************************/
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