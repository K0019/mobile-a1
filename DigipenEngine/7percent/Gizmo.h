#pragma once
/******************************************************************************/
/*!
\file   Gizmo.h
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Class that handles Gizmo operations for the editor. Implemented in ImGui.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#ifdef IMGUI_ENABLED

enum class GizmoType {
    None,
    Translate,
    Rotate,
    Scale
};

class Gizmo {
public:
    static constexpr float GIZMO_SIZE = 100.0f;
    static constexpr float HANDLE_SIZE = 10.0f;
    static constexpr float ROTATION_RADIUS = 50.0f;
    
    Gizmo();

    void attach(Transform& transform);

    void detach();

    void draw(ImDrawList* viewport);

    void processInput();

    void setType(GizmoType type);

    Transform* getAttachedTransform() const;
    bool isAttached() const;

private:
    GizmoType m_activeType;
    bool m_isDragging;
    int m_selectedAxis;  // -1: none, 0: x, 1: y, 2: both (for scale uniform)
    int m_hoveredAxis = -1;  // -1: none, 0: x, 1: y, 2: both (for scale uniform)
    Vector2 m_dragStart;
    Vector2 m_initialPosition;
    float m_initialRotation;
    Vector2 m_initialScale;
    Transform* m_attachedTransform;

    ImU32 getAxisColor(int axis, ImU32 baseColor) const;

    float getScaledHandleSize() const;

    float getScaledRotationRadius() const;

    float getScaledGizmoSize() const;

    void handleInput();

    void drawTranslationGizmo(ImDrawList* drawList, const Vector2& center);

    void drawRotationGizmo(ImDrawList* drawList, const Vector2& center, float rotation);

    void drawScaleGizmo(ImDrawList* drawList, const Vector2& center, float rotation);

    bool isPointNearLine(Vector2 point, Vector2 line_start, Vector2 line_end, float threshold);

    bool isPointInRect(const Vector2& point, const Vector2& rectCenter, float size);

    void checkTranslationHandles(const Vector2& mousePos, const Vector2& center);

    void checkRotationHandle(const Vector2& mousePos, const Vector2& center);

    void checkScaleHandles(const Vector2& mousePos, const Vector2& center, float rotation);
};
#endif