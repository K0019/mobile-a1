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
#include "ImguiHeader.h"
#include "ImGuizmo.h"
#include "EditorCameraBridge.h"   // for EditorCam_TryGet(...)
#include "EditorGizmoBridge.h"
#include "fa.h" 

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
    ~Gizmo();
    void attach(Transform& transform);

    void detach();

    void draw(ImDrawList* viewport);

    void processInput();

    void setType(GizmoType type);

    Transform* getAttachedTransform() const;
    bool isAttached() const;


    //TK
    void SetOperation(ImGuizmo::OPERATION op) { m_operation = op; }
    ImGuizmo::OPERATION GetOperation() const { return m_operation; }
    static constexpr ImGuizmo::OPERATION kNoneOp = (ImGuizmo::OPERATION)(-1);

private:
    ImGuizmo::OPERATION m_operation = kNoneOp;
    bool m_isDragging;
    int m_selectedAxis;  // -1: none, 0: x, 1: y, 2: both (for scale uniform)
    int m_hoveredAxis = -1;  // -1: none, 0: x, 1: y, 2: both (for scale uniform)
    Vec2 m_dragStart;
    Vec2 m_initialPosition;
    float m_initialRotation;
    Vec2 m_initialScale;
    Transform* m_attachedTransform;
    bool  m_snapEnabled = false;
    float m_translateSnap[3] = { 1.0f, 1.0f, 1.0f };  // units
    float m_scaleSnap[3] = { 0.1f, 0.1f, 0.1f };  // scale step
    float m_rotateSnapDeg = 15.0f;                 // degrees
    float m_gizmoSizeClip = 0.12f;                 // ImGuizmo::SetGizmoSizeClipSpace (default ~0.1)
    bool  m_mouseOverScene = false;                 // optional: for gating input/camera

    ImU32 getAxisColor(int axis, ImU32 baseColor) const;

    float getScaledHandleSize() const;

    float getScaledRotationRadius() const;

    float getScaledGizmoSize() const;

    void handleInput();

    void drawTranslationGizmo(ImDrawList* drawList, const Vec2& center);

    void drawRotationGizmo(ImDrawList* drawList, const Vec2& center, float rotation);

    void drawScaleGizmo(ImDrawList* drawList, const Vec2& center, float rotation);

    bool isPointNearLine(Vec2 point, Vec2 line_start, Vec2 line_end, float threshold);

    bool isPointInRect(const Vec2& point, const Vec2& rectCenter, float size);

    void checkTranslationHandles(const Vec2& mousePos, const Vec2& center);

    void checkRotationHandle(const Vec2& mousePos, const Vec2& center);

    void checkScaleHandles(const Vec2& mousePos, const Vec2& center, float rotation);
};
#endif