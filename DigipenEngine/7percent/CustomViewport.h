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

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#pragma once
#include "Engine.h"
#include "imgui_internal.h"
#include "ryan-c/Renderer.h"

/**
 * @brief The CustomViewport class represents a custom viewport for rendering graphics.
 */
class CustomViewport
{
    friend class ST<CustomViewport>;

#ifdef IMGUI_ENABLED
    using Vec = ImVec2;
#else
    using Vec = Vector2;
#endif

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
#ifdef IMGUI_ENABLED
    void DrawPlayControls();
    /**
     * @brief DrawImGuiWindow draws the ImGui window for the custom viewport.
     */
    void DrawImGuiWindow();

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
    Vector2 WindowToWorldPosition(const Vector2& inWindowPos) const;

    bool IsMouseInViewport(const Vector2& mousePos) const;

    /**
     * @brief SetDisableMoving sets whether moving the viewport is disabled.
     * @param disable True to disable moving the viewport, false otherwise.
     */
    void SetDisableMoving(bool disable);

    /**
     * @brief GetViewportRenderSize returns the render size of the viewport.
     * @return The render size of the viewport.
     */
    Vector2 GetViewportRenderSize() const;

     ~CustomViewport() = default;

    std::string name {ICON_FA_GAMEPAD " Scene"}; /**< The name of the ImGui Window. Specifically put here because I use it more than once*/
private:
    /**
     * @brief Default constructor for the CustomViewport class.
     */
    CustomViewport() = default;

    bool disableMoving{ false }; /**< Flag indicating whether moving the viewport is disabled. */
    unsigned width {},height {}; /**< The width and height of the viewport. */
    float aspect_ratio {}; /**< The aspect ratio of the viewport. */
    float titleBarHeight {}; /**< The height of the title bar of the viewport window. */
    Vec windowPosAbsolute; /**< The absolute position of the viewport window. */
    Vec contentMin; /**< The minimum position of the content region. */
    Vec contentMax; /**< The maximum position of the content region. */
    Vec viewportRenderSize; /**< The render size of the viewport. */
    
};