/******************************************************************************/
/*!
\file   Gizmo.cpp
\par    Project: 7percent
\par    Course: CSD2401
\par    Section B
\par    Software Engineering Project 3
\date   09/25/2024

\author Ryan Cheong (100%)
\par    email: ngaihangryan.cheong\@digipen.edu
\par    DigiPen login: ngaihangryan.cheong

\brief
Definition of Gizmo class.

All content © 2024 DigiPen Institute of Technology Singapore.
All rights reserved.
*/
/******************************************************************************/
#ifdef IMGUI_ENABLED
#include "Gizmo.h"
#include "CustomViewport.h"
#include "imgui_internal.h"
#include "EditorHistory.h"

namespace {
    // PI constant from GLM for angle calculations
    constexpr float PI = glm::pi<float>(); // 3.14159265359f

    // Base colors for gizmo axes (using RGB values from 0-255)
    // Slightly dimmed from full intensity (255) to allow for brightening on hover/select
    ImU32 X_axis = IM_COL32(200, 0, 0, 255);     // Red for X
    ImU32 Y_axis = IM_COL32(0, 200, 0, 255);     // Green for Y
    ImU32 Uniform_axis = IM_COL32(0, 0, 200, 255); // Blue for uniform scale/ rotation handle
}

Gizmo::Gizmo()
    : m_operation(kNoneOp)
    , m_isDragging(false)
    , m_hoveredAxis(-1)
    , m_selectedAxis(-1)
    , m_attachedTransform(nullptr)
{
}

Gizmo::~Gizmo() = default;


static ImGuizmo::OPERATION ToImguiOp(GizmoType t)
{
    switch (t)
    {
    case GizmoType::Translate: return ImGuizmo::TRANSLATE;
    case GizmoType::Rotate:    return ImGuizmo::ROTATE;
    case GizmoType::Scale:     return ImGuizmo::SCALE;
    default:                   return (ImGuizmo::OPERATION)(-1); // kNoneOp
    }
}

void Gizmo::attach(Transform& transform)
{
    // If we’re already attached to the same transform, nothing to do.
    if (m_attachedTransform == &transform)
        return;

    m_attachedTransform = &transform;

    // Reset transient interaction state
    m_isDragging = false;
    m_hoveredAxis = -1;
    m_selectedAxis = -1;

    // (Optional) choose a default mode on first attach
    if (m_operation == kNoneOp)
        m_operation = ImGuizmo::TRANSLATE;
}

void Gizmo::detach()
{
    if (!m_attachedTransform)
        return;

    // End any active interaction and drop the reference
    m_isDragging = false;
    m_hoveredAxis = -1;
    m_selectedAxis = -1;
    m_attachedTransform = nullptr;

    // If your old code disabled viewport panning while dragging,
    // this is the place to re-enable it (no-op if you don't use it):
    // ST<CustomViewport>::Get()->SetDisableMoving(false);
}

//void Gizmo::draw([[maybe_unused]] ImDrawList* /*viewport*/)
void Gizmo::draw(ImDrawList* /*viewport*/)
{
    //// Bail if nothing is attached
    //if (!m_attachedTransform) return;

    //// --- pull current operation & mode from the editor bridge ---
    //ImGuizmo::OPERATION op = EditorGizmo_Op();     // returns ImGuizmo::OPERATION
    //ImGuizmo::MODE      mode = EditorGizmo_Mode(); // WORLD/LOCAL

    //// If the editor hasn't published anything yet, default once and publish it
    //if (op == (ImGuizmo::OPERATION)(-1)) {
    //    op = ImGuizmo::TRANSLATE;
    //    EditorGizmo_Publish(op, mode);
    //}

    //// (optional debug)
    //// CONSOLE_LOG(LEVEL_INFO) << "Gizmo::draw op=" << (int)op << " mode=" << (int)mode;

    //// --- ImGuizmo begin frame / drawlist / rect (unchanged) ---
    //ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());
    //static int s_lastFrame = -1;
    //const int curFrame = ImGui::GetFrameCount();
    //if (s_lastFrame != curFrame) { ImGuizmo::BeginFrame(); s_lastFrame = curFrame; }
    //ImGuizmo::Enable(true);

    //const char* kSceneWindowTitle = ICON_FA_GAMEPAD " Scene";
    //ImGuiWindow* sceneWin = ImGui::FindWindowByName(kSceneWindowTitle);
    //if (!sceneWin || !sceneWin->WasActive) return;

    //const ImRect& inner = sceneWin->InnerRect;
    //ImGuizmo::SetDrawlist(sceneWin->DrawList);
    //ImGuizmo::SetRect(inner.Min.x, inner.Min.y, inner.GetWidth(), inner.GetHeight());

    //// --- camera from bridge ---
    //glm::mat4 V(1.f), P(1.f);
    //bool isOrtho = false;
    //if (!EditorCam_TryGet(V, P, isOrtho)) return;
    //ImGuizmo::SetOrthographic(isOrtho);

    //float view[16], proj[16];
    //std::memcpy(view, glm::value_ptr(V), sizeof(view));
    //std::memcpy(proj, glm::value_ptr(P), sizeof(proj));

    //// --- model from attached transform ---
    //glm::mat4 M(1.f);
    //m_attachedTransform->SetMat4ToWorld(&M);

    //float model[16];
    //std::memcpy(model, glm::value_ptr(M), sizeof(model));

    //ImGuizmo::SetID((int)(intptr_t)m_attachedTransform);
    //ImGuizmo::Manipulate(view, proj, op, mode, model, nullptr, nullptr);

    //// legacy flags
    //m_isDragging = ImGuizmo::IsUsing();
    //m_hoveredAxis = ImGuizmo::IsOver() ? 0 : -1;

    //// --- writeback (translate/rotate/scale) ---
    //if (ImGuizmo::IsUsing())
    //{
    //    float t[3], r[3], s[3];
    //    ImGuizmo::DecomposeMatrixToComponents(model, t, r, s);
    //    const Transform* parent = m_attachedTransform->GetParent();

    //    auto setLocalFromWorld = [&](auto setterLocal) {
    //        glm::mat4 parentW(1.f);
    //        parent->SetMat4ToWorld(&parentW);
    //        glm::mat4 worldM = glm::make_mat4(model);
    //        glm::mat4 localM = glm::inverse(parentW) * worldM;
    //        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(localM), t, r, s);
    //        setterLocal();
    //        };

    //    switch (op)
    //    {
    //    case ImGuizmo::TRANSLATE:
    //        if (mode == ImGuizmo::WORLD || !parent)
    //            m_attachedTransform->SetWorldPosition({ t[0], t[1], t[2] });
    //        else
    //            setLocalFromWorld([&] { m_attachedTransform->SetLocalPosition({ t[0], t[1], t[2] }); });
    //        break;

    //    case ImGuizmo::ROTATE:
    //        if (mode == ImGuizmo::WORLD || !parent)
    //            m_attachedTransform->SetWorldRotation({ r[0], r[1], r[2] });
    //        else
    //            setLocalFromWorld([&] { m_attachedTransform->SetLocalRotation({ r[0], r[1], r[2] }); });
    //        break;

    //    case ImGuizmo::SCALE:
    //        if (mode == ImGuizmo::WORLD || !parent)
    //            m_attachedTransform->SetWorldScale({ s[0], s[1], s[2] });
    //        else
    //            setLocalFromWorld([&] { m_attachedTransform->SetLocalScale({ s[0], s[1], s[2] }); });
    //        break;

    //    default: break;
    //    }
    //}
}
void Gizmo::processInput()
{
    //if(m_activeType == GizmoType::None || !m_attachedTransform) return;
    //handleInput();
}

void Gizmo::setType(GizmoType type) {
    m_operation = ToImguiOp(type);       // translate legacy type -> ImGuizmo op
    m_isDragging = false;
    m_selectedAxis = -1;
}
Transform* Gizmo::getAttachedTransform() const {
    return m_attachedTransform;
}
bool Gizmo::isAttached() const {
    return m_attachedTransform != nullptr;
}
ImU32 Gizmo::getAxisColor(int axis, ImU32 baseColor) const {
    if(m_selectedAxis == axis) {
        // Selected state: brighten the color significantly
        // Use bit shifts to extract RGB components and clamp result to 255
        return IM_COL32(
            std::min(static_cast<int>((baseColor >> IM_COL32_R_SHIFT & 0xFF) * 1.5f), 255),
            std::min(static_cast<int>((baseColor >> IM_COL32_G_SHIFT & 0xFF) * 1.5f), 255),
            std::min(static_cast<int>((baseColor >> IM_COL32_B_SHIFT & 0xFF) * 1.5f), 255),
            255
        );
    }
    if(m_hoveredAxis == axis) {
        // Hover state: slightly brighten
        return IM_COL32(
            std::min(static_cast<int>((baseColor >> IM_COL32_R_SHIFT & 0xFF) * 1.2f), 255),
            std::min(static_cast<int>((baseColor >> IM_COL32_G_SHIFT & 0xFF) * 1.2f), 255),
            std::min(static_cast<int>((baseColor >> IM_COL32_B_SHIFT & 0xFF) * 1.2f), 255),
            255
        );
    }
    return baseColor;
}

float Gizmo::getScaledHandleSize() const {
    return 0;
}

float Gizmo::getScaledRotationRadius() const {
    return 0;
}

float Gizmo::getScaledGizmoSize() const {
    return 0;
}

void Gizmo::handleInput() {
    if(!m_attachedTransform) return;

    CONSOLE_LOG_UNIMPLEMENTED() << "Gizmo handle input";

    //Vec2 mousePos = { ImGui::GetMousePos().x, ImGui::GetMousePos().y };
    //Transform screenTransform = ST<CustomViewport>::Get()->WorldToWindowTransform(*m_attachedTransform);
    //Vec2 screenPos = screenTransform.GetWorldPosition();
    //bool isMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);

    //if(!m_isDragging) {
    //    // Update hover state
    //    switch(m_activeType) {
    //        case GizmoType::Translate:
    //            checkTranslationHandles(mousePos, screenPos);
    //            break;
    //        case GizmoType::Rotate:
    //            checkRotationHandle(mousePos, screenPos);
    //            break;
    //        case GizmoType::Scale:
    //            checkScaleHandles(mousePos, screenPos, screenTransform.GetWorldRotation());
    //            break;
    //    }

    //    if(isMouseDown && m_hoveredAxis != -1) {
    //        m_isDragging = true;
    //        m_selectedAxis = m_hoveredAxis;
    //        m_dragStart = mousePos;  // Store screen space position for translation/rotation
    //        m_initialScale = m_attachedTransform->GetWorldScale();
    //        ST<CustomViewport>::Get()->SetDisableMoving(true);
    //    }
    //}
    //else if(m_isDragging) {
    //    if(isMouseDown) {
    //        switch(m_activeType) {
    //            case GizmoType::Translate: {
    //                // Get current position before modification
    //                m_initialPosition = m_attachedTransform->GetWorldPosition();
    //                Vec2 screenDelta = mousePos - m_dragStart;
    //                screenDelta.y *= -1;  // Invert Y because screen space is Y-down

    //                // Constrain movement to selected axis
    //                if(m_selectedAxis == 0) screenDelta.y = 0;  // X axis only
    //                if(m_selectedAxis == 1) screenDelta.x = 0;  // Y axis only

    //                // Apply translation in screen space
    //                ST<History>::Get()->IntermediateEvent(HistoryEvent_Translation{ m_attachedTransform->GetEntity(), m_attachedTransform->GetLocalPosition() });
    //                m_attachedTransform->SetWorldPosition(m_initialPosition + screenDelta);
    //                m_dragStart = mousePos;  // Update drag start for next frame
    //                break;
    //            }
    //            case GizmoType::Rotate: {
    //                // Calculate vector from center to mouse
    //                m_initialRotation = m_attachedTransform->GetWorldRotation();
    //                Vec2 toMouse = mousePos - screenPos;
    //                
    //                // Get angle in degrees from vector components
    //                // atan2 gives angle in radians from [-pi, pi]
    //                float rotation = atan2f(toMouse.y, toMouse.x) * -180.0f / PI;
    //                
    //                // Convert from [-180, 180] to [0, 360] range
    //                if(rotation < 0) {
    //                    rotation += 360.0f;
    //                }

    //                // Check for snapping
    //                const float SNAP_THRESHOLD = 5.0f;  // Degrees within which to snap
    //                const float SNAP_ANGLE = 90.0f;     // Snap to multiples of this angle
    //                
    //                // Find nearest multiple of 90 degrees
    //                float nearestSnap = roundf(rotation / SNAP_ANGLE) * SNAP_ANGLE;
    //                float distToSnap = fabsf(rotation - nearestSnap);
    //                if(distToSnap <= SNAP_THRESHOLD) {
    //                    rotation = nearestSnap;
    //                }

    //                ST<History>::Get()->IntermediateEvent(HistoryEvent_Rotation{ m_attachedTransform->GetEntity(), m_attachedTransform->GetLocalRotation() });
    //                m_attachedTransform->SetWorldRotation(rotation);
    //                break;
    //            }

    //            case GizmoType::Scale: {
    //                Vec2 screenDelta = mousePos - m_dragStart;
    //                Vec2 rotatedDelta;
    //                Vec2 newScale{ m_initialScale };
    //                // Convert rotation to radians for trigonometric functions
    //                float rotRad = m_attachedTransform->GetWorldRotation() * PI / 180.0f;
    //                if(m_selectedAxis == 0) { // X axis scaling
    //                    // Project screen delta onto rotated X axis
    //                    rotatedDelta.x = screenDelta.x * cosf(rotRad) + screenDelta.y * sinf(rotRad);
    //                    // Calculate scale multiplier based on drag distance
    //                    float scaleMultiplier = 1.0f + rotatedDelta.x / (GIZMO_SIZE * 2.0f);
    //                    newScale.x *= scaleMultiplier;
    //                }
    //                else if(m_selectedAxis == 1) { // Y axis scaling
    //                    // Project screen delta onto rotated Y axis
    //                    rotatedDelta.y = -screenDelta.x * sinf(rotRad) + screenDelta.y * -cosf(rotRad);
    //                    float scaleMultiplier = 1.0f + rotatedDelta.y / (GIZMO_SIZE * 2.0f);
    //                    newScale.y *= scaleMultiplier;
    //                }
    //                else if(m_selectedAxis == 2) { // Uniform scaling
    //                    // Use combined X and Y movement for uniform scaling
    //                    Vec2 centerPos = ST<CustomViewport>::Get()->WorldToWindowTransform(*m_attachedTransform).GetWorldPosition();
    //                    Vec2 toInitialMouse = m_dragStart - centerPos;
    //                    Vec2 toCurrentMouse = mousePos - centerPos;

    //                    float initialDist = toInitialMouse.Length();

    //                    if(initialDist > 1.0f) {
    //                        // Normalize the initial direction vector
    //                        Vec2 initialDir = toInitialMouse.Normalize();

    //                        // Project current mouse position onto the initial direction vector
    //                        float projectedDist = toCurrentMouse.Dot(initialDir);

    //                        // Clamp the projected distance to prevent scaling when moving past center
    //                        projectedDist = std::max(0.0f, projectedDist);

    //                        // Calculate scale multiplier based on projected distance
    //                        float scaleMultiplier = projectedDist / initialDist;

    //                        // Add a minimum scale to prevent collapse
    //                        const float MIN_SCALE = 0.01f;
    //                        float minMultiplier = MIN_SCALE / m_initialScale.x;
    //                        scaleMultiplier = std::max(scaleMultiplier, minMultiplier);

    //                        newScale *= scaleMultiplier;
    //                    }
    //                }
    //                ST<History>::Get()->IntermediateEvent(HistoryEvent_Scale{ m_attachedTransform->GetEntity(), m_attachedTransform->GetLocalScale() });
    //                m_attachedTransform->SetWorldScale(newScale);
    //                break;
    //            }
    //            default: break;
    //        }
    //    }
    //    else {
    //        m_isDragging = false;
    //        m_selectedAxis = -1;
    //        m_hoveredAxis = -1;
    //        ST<CustomViewport>::Get()->SetDisableMoving(false);
    //    }
    //}
}

void Gizmo::drawTranslationGizmo(ImDrawList* drawList, const Vec2& center) {
    float axisLength = getScaledGizmoSize();
    float handleSize = getScaledHandleSize();

    // X axis (red)
    Vec2 xAxisEnd = center + Vec2(axisLength, 0);
    ImU32 xAxisColor = getAxisColor(0, X_axis);

    drawList->AddLine(
        { center.x, center.y },
        { xAxisEnd.x, xAxisEnd.y },
        xAxisColor,
        2.0f
    );

    drawList->AddCircleFilled(
        { xAxisEnd.x, xAxisEnd.y },
        handleSize,
        xAxisColor
    );

    // Y axis (green)
    Vec2 yAxisEnd = center + Vec2(0, -axisLength);
    ImU32 yAxisColor = getAxisColor(1, Y_axis);

    drawList->AddLine(
        { center.x, center.y },
        { yAxisEnd.x, yAxisEnd.y },
        yAxisColor,
        2.0f
    );

    drawList->AddCircleFilled(
        { yAxisEnd.x, yAxisEnd.y },
        handleSize,
        yAxisColor
    );

    // Center handle for free movement
    ImU32 centerColor = getAxisColor(2, IM_COL32(180, 180, 180, 200));
    drawList->AddCircleFilled(
        { center.x, center.y },
        handleSize - (m_selectedAxis == 2 ? 0 : 2),
        centerColor
    );
}
void Gizmo::drawRotationGizmo(ImDrawList* drawList, const Vec2& center, float rotation) {
    float radius = getScaledRotationRadius();
    float handleSize = getScaledHandleSize();
    ImU32 rotationColor = getAxisColor(0, Uniform_axis);
    rotation *= -PI / 180.0f;  // Convert degrees to radians

    drawList->AddCircle(
        { center.x, center.y },
        radius,
        rotationColor,
        32,
        2.0f
    );

    Vec2 rotationHandle = center + Vec2(
        radius * cosf(rotation),
        radius * sinf(rotation)
    );

    drawList->AddCircleFilled(
        { rotationHandle.x, rotationHandle.y },
        handleSize,
        rotationColor
    );
}
void Gizmo::drawScaleGizmo(ImDrawList* drawList, const Vec2& center, float rotation) {
    float axisLength = getScaledGizmoSize();
    float handleSize = getScaledHandleSize();
    // Convert rotation from degrees to radians and negate for correct direction
    // Negative because screen space Y is inverted (increases downward)
    float rotRad = rotation * -PI / 180.0f;

    // Calculate X axis direction vector using rotation
    // [cos(rad), sin(rad)] gives a unit vector rotated by rad radians
    Vec2 xDirection(cosf(rotRad), sinf(rotRad));
    Vec2 xAxisEnd = center + xDirection * axisLength;
    ImU32 xScaleColor = getAxisColor(0, X_axis);

    drawList->AddLine(
        { center.x, center.y },
        { xAxisEnd.x, xAxisEnd.y },
        xScaleColor,
        2.0f
    );

    drawList->AddRectFilled(
        { xAxisEnd.x - handleSize, xAxisEnd.y - handleSize },
        { xAxisEnd.x + handleSize, xAxisEnd.y + handleSize },
        xScaleColor
    );

    // Y axis direction is perpendicular to X axis
    // [sin(rad), -cos(rad)] gives a unit vector rotated by 90 degrees counterclockwise 
    Vec2 yDirection(sinf(rotRad), -cosf(rotRad)); // Perpendicular to xDirection
    Vec2 yAxisEnd = center + yDirection * axisLength;
    ImU32 yScaleColor = getAxisColor(1, Y_axis);

    drawList->AddLine(
        { center.x, center.y },
        { yAxisEnd.x, yAxisEnd.y },
        yScaleColor,
        2.0f
    );

    drawList->AddRectFilled(
        { yAxisEnd.x - handleSize, yAxisEnd.y - handleSize },
        { yAxisEnd.x + handleSize, yAxisEnd.y + handleSize },
        yScaleColor
    );

    CONSOLE_LOG_UNIMPLEMENTED() << "Gizmo draw scale";

    //// Uniform scale handle at rotated corner
    //Transform screenTransform = ST<CustomViewport>::Get()->WorldToWindowTransform(*m_attachedTransform);
    //Vec2 scale = screenTransform.GetWorldScale();

    //// Place handle at corner by adding half scale in both axis directions
    //Vec2 cornerOffset = (xDirection * scale.x + yDirection * scale.y) * 0.5f;
    //Vec2 uHandlePos = center + cornerOffset;

    //ImU32 uScaleColor = getAxisColor(2, Uniform_axis);
    //drawList->AddRectFilled(
    //    { uHandlePos.x - handleSize, uHandlePos.y - handleSize },
    //    { uHandlePos.x + handleSize, uHandlePos.y + handleSize },
    //    uScaleColor
    //);
}

// Helper function for line hit detection
bool Gizmo::isPointNearLine(Vec2 point, Vec2 line_start, Vec2 line_end, float threshold) {
    Vec2 toMouse = point - line_start;      // Vector from line start to mouse
    Vec2 toEnd = line_end - line_start;     // Vector representing the line
    float lineLength = toEnd.Length();         // Length of the line
    float dot = toMouse.Dot(toEnd) / lineLength;  // Projected distance along line

    // Check if projection falls outside line segment
    if(dot < 0 || dot > lineLength) return false;

    // Calculate closest point on line to mouse
    Vec2 projection = line_start + toEnd * (dot / lineLength);
    Vec2 toProjection = point - projection;

    // Check if distance to line is within threshold
    return toProjection.Length() < threshold;
}

// Helper function for square hit detection around a point
bool Gizmo::isPointInRect(const Vec2& point, const Vec2& rectCenter, float size) {
    // Simple AABB check with square region around center point
    return point.x >= rectCenter.x - size && point.x <= rectCenter.x + size &&
        point.y >= rectCenter.y - size && point.y <= rectCenter.y + size;
}

void Gizmo::checkTranslationHandles(const Vec2& mousePos, const Vec2& center) {
    float axisLength = getScaledGizmoSize();
    float handleSize = getScaledHandleSize();
    m_hoveredAxis = -1;  // Reset hover state

    // Check center handle first (takes precedence)
    if(isPointInRect(mousePos, center, handleSize)) {
        m_hoveredAxis = 2;  // 2 indicates free movement (both axes)
        // Only update selected axis if not dragging
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
        return;
    }

    // Check X axis line
    Vec2 xAxisEnd = center + Vec2(axisLength, 0);
    if(isPointNearLine(mousePos, center, xAxisEnd, handleSize)) {
        m_hoveredAxis = 0;
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
        return;
    }

    // Check Y axis line
    Vec2 yAxisEnd = center + Vec2(0, -axisLength);
    if(isPointNearLine(mousePos, center, yAxisEnd, handleSize)) {
        m_hoveredAxis = 1;
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
    }
}
void Gizmo::checkRotationHandle(const Vec2& mousePos, const Vec2& center) {
    float radius = getScaledRotationRadius();
    float handleSize = getScaledHandleSize();
    m_hoveredAxis = -1;

    // Check if mouse is near the rotation circle
    Vec2 toMouse = mousePos - center;
    float dist = toMouse.Length();  // Distance from center to mouse
    float radiusDiff = fabsf(dist - radius);  // How far mouse is from circle

    // If within handle thickness
    if(radiusDiff < handleSize) {
        m_hoveredAxis = 0;
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
    }
}

void Gizmo::checkScaleHandles(const Vec2& mousePos, const Vec2& center, float rotation) {
    float axisLength = getScaledGizmoSize();
    float handleSize = getScaledHandleSize();
    float rotRad = rotation * -PI / 180.0f;  // Convert degrees to radians
    m_hoveredAxis = -1;

    // X axis handle - follows rotation
    Vec2 xDirection(cosf(rotRad), sinf(rotRad));
    Vec2 xAxisEnd = center + xDirection * axisLength;
    if(isPointInRect(mousePos, xAxisEnd, handleSize)) {
        m_hoveredAxis = 0;
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
        return;
    }

    // Y axis handle - perpendicular to X
    Vec2 yDirection(sinf(rotRad), -cosf(rotRad));
    Vec2 yAxisEnd = center + yDirection * axisLength;
    if(isPointInRect(mousePos, yAxisEnd, handleSize)) {
        m_hoveredAxis = 1;
        m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
        return;
    }

    CONSOLE_LOG_UNIMPLEMENTED() << "Gizmo check scale handles";

    //// Uniform scale handle at rotated corner
    //Transform screenTransform = ST<CustomViewport>::Get()->WorldToWindowTransform(*m_attachedTransform);
    //Vec2 scale = screenTransform.GetWorldScale();
    //Vec2 cornerOffset = (xDirection * scale.x + yDirection * scale.y) * 0.5f;
    //Vec2 uHandlePos = center + cornerOffset;

    //if(isPointInRect(mousePos, uHandlePos, handleSize)) {
    //    m_hoveredAxis = 2;
    //    m_selectedAxis = m_isDragging ? m_selectedAxis : -1;
    //}
}
#endif