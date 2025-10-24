/******************************************************************************/
/*!
\file   CustomViewport.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
This file contains the declaration of the CustomViewport class, which represents a custom viewport for rendering graphics.

All content � 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Editor/Containers/GUIAsECS.h"
#include "Engine/Engine.h"
#include "ImGui/ImguiHeader.h"
#include "Editor/Gizmo.h"
#include "math/camera.h"

/**
 * @brief The CustomViewport class represents a custom viewport for rendering graphics.
 */
class CustomViewport : public editor::WindowBase<CustomViewport, false>
{
#ifdef IMGUI_ENABLED
    using Vec = ImVec2;
#else
    using Vec = Vec2;
#endif

public:
    CustomViewport(unsigned int width, unsigned int height);

    void DrawContainer(int id) override;
    void DrawWindow() override;

public:
    /**
     * @brief Init initializes the custom viewport with the specified width and height.
     * @param newWidth The new width of the viewport.
     * @param newHeight The new height of the viewport.
     */
    void Init(unsigned newWidth, unsigned newHeight);

    /**
     * @brief Resize resizes the custom viewport to the specified width and height.
     * @param newWidth The new width of the viewport.
     * @param newHeight The new height of the viewport.
     */
    void Resize(unsigned newWidth, unsigned newHeight);

    /**
     * @brief Polls input and updates the viewport camera accordingly.
     */
    void UpdateCameraControl();
#ifdef IMGUI_ENABLED
    void DrawPlayControls();

    /**
     * @brief MaintainAspectRatio is a callback function used to maintain the aspect ratio of the viewport.
     * @param data The ImGuiSizeCallbackData containing the size information.
     */
    static void MaintainAspectRatio(ImGuiSizeCallbackData* data);
#endif
    /**
     * @brief WorldToWindowTransform converts a world transform to a window transform.
     * @param worldTransform The world transform to convert.
     * @return The converted window transform.
     */
    Transform WorldToWindowTransform(const Transform& worldTransform) const;

    /**
     * @brief WindowToWorldPosition converts a window position to a world position.
     * @param inWindowPos The window position to convert.
     * @return The converted world position.
     */
    Vec3 WindowToWorldPosition(const Vec2& inWindowPos) const;

    bool IsMouseInViewport(const Vec2& mousePos) const;

    /**
     * @brief GetViewportRenderSize returns the render size of the viewport.
     * @return The render size of the viewport.
     */
    Vec2 GetViewportRenderSize() const;

    void updateCurrentEntity(ecs::EntityHandle entity);
    Camera GetViewportCamera() const;

    std::string name; /**< The name of the ImGui Window. Specifically put here because I use it more than once*/
private:
    unsigned width {},height {}; /**< The width and height of the viewport. */
    float aspect_ratio {}; /**< The aspect ratio of the viewport. */
    float titleBarHeight {}; /**< The height of the title bar of the viewport window. */
    Vec windowPosAbsolute; /**< The absolute position of the viewport window. */
    Vec contentMin; /**< The minimum position of the content region. */
    Vec contentMax; /**< The maximum position of the content region. */
    Vec viewportRenderSize; /**< The render size of the viewport. */
#ifdef IMGUI_ENABLED
    Gizmo m_gizmo;
#endif
    CameraPositioner_FirstPerson camera;

};
